//
// Created by qiaopc on 2021/8/30.
//

#include "MyAudio.h"

MyAudio::MyAudio(Playstatus *playstatus, int sample_rate, CallJava *callJava) {
    this->callJava = callJava;
    this->playstatus = playstatus;
    this->sample_rate = sample_rate;

    this->isCut = false;
    this->end_time = 0;
    this->showPcm = false;

    queue = new MyQueue(playstatus);
    buffer = (uint8_t *) av_malloc(sample_rate * 2 * 2);//采样率*声道数*位数大小（16/8）

    sampleBuffer = static_cast<SAMPLETYPE *>(malloc(sample_rate * 2 * 2)); //采样率 * 声道数 * 位数大小
    soundTouch = new SoundTouch();
    soundTouch->setSampleRate(sample_rate);
    soundTouch->setChannels(2);
    soundTouch->setPitch(pitch); //设置音调
    soundTouch->setTempo(speed); //设置音速
}

MyAudio::~MyAudio() {

}

void *decodePlay(void *data) {
    MyAudio *audio = (MyAudio *)(data);
//    audio->resampleAudio();
    audio->initOpenSLES();
    pthread_exit(&audio->thread_play);
}

void *pcmCallBack(void *data) {
    LOGD("pcmCallBack(void *data) ");
    MyAudio *audio = static_cast<MyAudio *>(data);
    audio->bufferQueue = new MyBufferQueue(audio->playstatus);

    while (audio->playstatus != NULL && !audio->playstatus->exit) {
        MyPcmBean *pcmBean = NULL;
        audio->bufferQueue->getBuffer(&pcmBean);
        if (pcmBean == NULL) {
            LOGD("pcmBean == NULL");
            continue;
        }
        LOGD("pcmbean buffer size is %d", pcmBean->buffsize);
        if (pcmBean->buffsize <= audio->defaultPcmSize) { //不用分包
            if (audio->isRecordPcm) {
                audio->callJava->onCallPcmToAAc(CHILD_THREAD, pcmBean->buffsize, pcmBean->buffer);
            }
            if(audio->showPcm) {
                audio->callJava->onCallPcmInfo(pcmBean->buffer, pcmBean->buffsize);
            }
        } else { //需要分包
            int pack_num = pcmBean->buffsize / audio->defaultPcmSize;
            int pack_sub = pcmBean->buffsize % audio->defaultPcmSize;
            for (int i = 0; i < pack_num; i++) {
                char *bf = static_cast<char *>(malloc(audio->defaultPcmSize));
                memcpy(bf, pcmBean->buffer + i * audio->defaultPcmSize, audio->defaultPcmSize);
                if (audio->isRecordPcm) {
                    LOGD("需要分包 %d  ...  %d", audio->defaultPcmSize, sizeof(bf));
                    audio->callJava->onCallPcmToAAc(CHILD_THREAD, audio->defaultPcmSize, bf);
                }
                if(audio->showPcm) {
                    audio->callJava->onCallPcmInfo(bf, pcmBean->buffsize);
                }
                free(bf);
                bf = NULL;
            }
            if (pack_sub > 0) {
                char *bf = static_cast<char *>(malloc(pack_sub));
                memcpy(bf, pcmBean->buffer + pack_num * audio->defaultPcmSize, pack_sub);
                if (audio->isRecordPcm) {
                    LOGD("需要分包pack_sub %d  ...  %d", pack_sub, sizeof(bf));
                    audio->callJava->onCallPcmToAAc(CHILD_THREAD, pack_sub, bf);
                }
                if(audio->showPcm) {
                    audio->callJava->onCallPcmInfo(bf, pack_sub);
                }

//                free(bf);
//                bf = NULL;
            }
        }

        delete(pcmBean);
        pcmBean = NULL;
    }
    LOGD("pthread_exit(&audio->pcmCallBackThread)");
    pthread_exit(&audio->pcmCallBackThread);
}

void MyAudio::play() {
    LOGD("MyAudio::play()");
    bufferQueue = new MyBufferQueue(playstatus);
    pthread_create(&thread_play, NULL, decodePlay, this);
    pthread_create(&pcmCallBackThread, NULL, pcmCallBack, this);
}

//FILE *outFile = fopen("/mnt/shared/Other/mymusic.pcm", "w");
//FILE *outFile = fopen("/storage/emulated/0/Download/mymusic.pcm", "w");

//实现重采样
int MyAudio::resampleAudio(void **pcmbuf) {
    while (playstatus != NULL && !playstatus->exit) {

        if (playstatus->seek) {
            av_usleep(1000 * 100);
            continue;
        }

        if (queue->getQueueSize() == 0) { //加载中
            if (!playstatus->load) {
                playstatus->load = true;
                callJava->onCallLoad(CHILD_THREAD, true);
            }
            av_usleep(1000 * 100);
            continue;
        } else {
            if (playstatus->load) {
                playstatus->load = false;
                callJava->onCallLoad(CHILD_THREAD, false);
            }
        }

        if (readFrameFinish) {
            avPacket = av_packet_alloc();
            if (queue->getAvpacket(avPacket) != 0) {
                av_packet_free(&avPacket);
                av_free(avPacket);
                avPacket = NULL;
                continue;
            }
            ret = avcodec_send_packet(avCodecContext, avPacket); //一个avpacket里，可能存在多个frame
            if (ret != 0) {
                av_packet_free(&avPacket);
                av_free(avPacket);
                avPacket = NULL;
                continue;
            }
        }


        avFrame = av_frame_alloc();
        ret = avcodec_receive_frame(avCodecContext, avFrame);
        if (ret == 0) { //==0 没有读取完
            readFrameFinish = false;

            if (avFrame->channels > 0 && avFrame->channel_layout == 0) {//声道数: 比如立体声 channels = 2
                //根据声道数获取声道布局
                avFrame->channel_layout = av_get_default_channel_layout(avFrame->channels);
            } else if (avFrame->channels == 0 && avFrame->channel_layout > 0) {
                avFrame->channels = av_get_channel_layout_nb_channels(avFrame->channel_layout);
            }
            SwrContext *swr_ctx = NULL; //重采样结构体
            swr_ctx = swr_alloc_set_opts(NULL,
                                         AV_CH_LAYOUT_STEREO, //声道布局，立体声
                                         AV_SAMPLE_FMT_S16, //采样位数 16位
                                         avFrame->sample_rate, //采样率 输入输出保持一致
                                         avFrame->channel_layout, //声道布局
                                         (AVSampleFormat) avFrame->format,//采样位数
                                         avFrame->sample_rate, //采样率 输入输出保持一致
                                         NULL,
                                         NULL);
            if(!swr_ctx || swr_init(swr_ctx) < 0) {
                av_packet_free(&avPacket);
                av_free(avPacket);
                avPacket = NULL;
                av_frame_free(&avFrame);
                av_free(avFrame);
                avFrame = NULL;
                if (swr_ctx != NULL) {
                    swr_free(&swr_ctx);
                    swr_ctx = NULL;
                }
                readFrameFinish = true;
                continue;
            }

            nb = swr_convert(
                    swr_ctx,
                    &buffer, //输出数据
                    avFrame->nb_samples, //输出采样个数
                    (const uint8_t **) avFrame->data, //原始数据
                    avFrame->nb_samples // 输入采样个数
            );

            int out_channels = av_get_channel_layout_nb_channels(AV_CH_LAYOUT_STEREO);
            // 数据个数 * 声道数（2） * 采样位数（16/8）
            data_size = nb * out_channels * av_get_bytes_per_sample(AV_SAMPLE_FMT_S16);
//            fwrite(buffer, 1, data_size, outFile);
//            if (LOG_DEBUG) {
//                LOGE("data size is %d", data_size);
//            }
            now_time = avFrame->pts * av_q2d(time_base); //多少帧 * 每帧的时间

            if (now_time < clock) {
                now_time = clock;
            }
            clock = now_time;
            *pcmbuf = buffer; // 这里out_buffer用的和buffer同一个内存
//            av_packet_free(&avPacket); //avcodec_receive_frame里统一释放，所这里先注掉
//            av_free(avPacket);
//            avPacket = NULL;
            av_frame_free(&avFrame);
            av_free(avFrame);
            avFrame = NULL;
            swr_free(&swr_ctx);
            swr_ctx = NULL;
            break;
        } else {
            readFrameFinish = true;
            av_packet_free(&avPacket);
            av_free(avPacket);
            avPacket = NULL;
            av_frame_free(&avFrame);
            av_free(avFrame);
            avFrame = NULL;
            continue;
        }
    }

    return data_size;
}


int MyAudio::getSoundTouchData() {
    //循环获取
    while (playstatus != NULL && !playstatus->exit) {
        out_buffer = NULL;
        if (finished) {
            finished = false;
            data_size = resampleAudio(reinterpret_cast<void **>(&out_buffer));
            if (data_size > 0) {
                //因为soundtouch需要16位，所以这里8位转成16位
                for (int i = 0; i < data_size / 2 + 1; i++) {
                    sampleBuffer[i] = (out_buffer[i * 2] | (out_buffer[i * 2 + 1]) << 8);
                }
                soundTouch->putSamples(sampleBuffer, nb);
                num = soundTouch->receiveSamples(sampleBuffer, data_size /4); // 为什么要/4 11:52  因为返回的是采样个数，要除以2（双声道）再除以2（采样位数16字节 = 2位）
            } else {
                soundTouch->flush();
            }
        }
        if (num == 0) {
            finished = true;
            continue;
        } else {
            if (out_buffer == NULL) {
                num = soundTouch->receiveSamples(sampleBuffer, data_size /4);
                if (num == 0) {
                    finished = true;
                    continue;
                }
            }
            return num;
        }
    }
    return 0;
}

void pcmBufferCallBack(SLAndroidSimpleBufferQueueItf bf, void * context) {
    MyAudio *audio = static_cast<MyAudio *>(context);
    if(audio != NULL) {
        int buffersize = audio->getSoundTouchData(); // 返回的采样个数，sample个数
        if (buffersize > 0) {
            audio->clock += buffersize / (double)(audio->sample_rate * 2 * 2); //加上这一帧播放的时间

            if (audio->clock - audio->last_time >= 0.1) {
                audio->last_time = audio->clock;
                audio->callJava->onCallTimeInfo(CHILD_THREAD, audio->clock, audio->duration);
            }

            audio->bufferQueue->putBuffer(audio->sampleBuffer, buffersize * 4);

            audio->callJava->onCallValumeDB(CHILD_THREAD, audio->getPCMDB(
                    reinterpret_cast<char *>(audio->sampleBuffer), buffersize * 4));
            (*audio->pcmBufferQueue)->Enqueue(audio->pcmBufferQueue, audio->sampleBuffer, buffersize * 2 * 2);// buffersize * 2 * 2  sample个数 * 16字节=2位 * 双声道

            if (audio->isCut) {
                if (audio->clock > audio->end_time) {
                    LOGE("裁剪退出");
                    audio->playstatus->exit = true;
                }
            }
        }
    }
}

//初始化openSLES
void MyAudio::initOpenSLES() {
    SLresult result;

    //第一步 创建引擎对象
    result = slCreateEngine(&engineObject, 0, 0, 0, 0, 0);
    result = (*engineObject)->Realize(engineObject, SL_BOOLEAN_FALSE);
    result = (*engineObject)->GetInterface(engineObject, SL_IID_ENGINE, &engineEngine);

    //第二步 创建混音器
    const SLInterfaceID mids[1] = {SL_IID_ENVIRONMENTALREVERB};
    const SLboolean mreq[1] = {SL_BOOLEAN_FALSE};
    result = (*engineEngine)->CreateOutputMix(engineEngine, &outputMixObject, 1, mids, mreq);
    (void)result;// (void) result to avoid warning 有时会插入这种愚蠢的代码来消除编译器或 lint 警告，例如当断言检查关闭时“不使用变量结果”。
    result = (*outputMixObject)->Realize(outputMixObject, SL_BOOLEAN_FALSE);
    (void)result;
    result = (*outputMixObject)->GetInterface(outputMixObject, SL_IID_ENVIRONMENTALREVERB, &outputMixEnvironmentalReverb);
    if(SL_RESULT_SUCCESS == result) {
        result = (*outputMixEnvironmentalReverb)->SetEnvironmentalReverbProperties(outputMixEnvironmentalReverb, &reverbSettings);
        (void)result;
    }
    SLDataLocator_OutputMix outputMix = {SL_DATALOCATOR_OUTPUTMIX, outputMixObject};
    SLDataSink audioSnk = {&outputMix, 0};

    //第三步 配置PCM格式信息
    SLDataLocator_AndroidBufferQueue android_queue = {SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE, 2};
    SLDataFormat_PCM pcm = {
            SL_DATAFORMAT_PCM, // 播放pcm格式的数据
            2,// 2个声道（立体声）
            getCurrentSampleRateForOpensles(sample_rate), //44100hz的频率
            SL_PCMSAMPLEFORMAT_FIXED_16,//位数16位
            SL_PCMSAMPLEFORMAT_FIXED_16,//和位数一致
            SL_SPEAKER_FRONT_LEFT|SL_SPEAKER_FRONT_RIGHT, //立体声（前左前右）
            SL_BYTEORDER_LITTLEENDIAN //结束标志
    };
    SLDataSource slDataSource = {&android_queue, &pcm};


    //其中类型的GUID是OpenSLES定死的，音量(SL_IID_VOLUME)、采样率控制(SL_IID_PLAYBACKRATE)、均衡器(SL_IID_EQUALIZER)、预设混响(SL_IID_PRESETREVERB)、环境混响(SL_IID_ENVIRONMENTALREVERB)、3D定位(SL_IID_3DLOCATION)、多普勒效应(SL_IID_3DDOPPLER)、低音增强(SL_IID_BASSBOOST)、升降调(SL_IID_PITCH)、虚拟化(SL_IID_VIRTUALIZER)。这里没有你想要的？你想自定义？什么，你要做一个高音增强？无论做什么，都得在这里面选一个。为了简单一点，那就选虚拟化吧，虚拟化只有一个固定参数。(这里没看明白？那就把整个教程都看完，相信看到最后你会明白的)
    //　　下一步是生成一个自己独一无二的GUID来给自己的SoundFX命名。生成的办法有很多，有现成软件也有网页。这里我生成的是{42C6510E-1811-4857-8CA5-C204A8A3B0D4}。
    //　　以上提及的详细内容和编程指导请阅读Android NDK\platforms\android-14\arch-arm\usr\include\SLES\OpenSLES.h。(Android 4.0对应android
    //SL_IID_PLAYBACKRATE 自动控制采样率，避免有些音频播放有杂音
    const SLInterfaceID  ids[5] = {SL_IID_BUFFERQUEUE,SL_IID_EFFECTSEND, SL_IID_VOLUME, SL_IID_MUTESOLO, SL_IID_PLAYBACKRATE}; //opensl的inferface，需要的要写在里面，否则调用了也无效
    const SLboolean  req[5] = {SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE};

    (*engineEngine)->CreateAudioPlayer(engineEngine, &pcmPlayerObject, &slDataSource, &audioSnk, 5, ids, req);
    //初始化播放器
    (*pcmPlayerObject)->Realize(pcmPlayerObject, SL_BOOLEAN_FALSE);
    //得到接口后调用 获取Player接口
    (*pcmPlayerObject)->GetInterface(pcmPlayerObject, SL_IID_PLAY, &pcmPlayerPlay);
    //获取声音接口
    (*pcmPlayerObject)->GetInterface(pcmPlayerObject, SL_IID_VOLUME, &pcmVolumePlay);
    //获取声道接口
    (*pcmPlayerObject)->GetInterface(pcmPlayerObject, SL_IID_MUTESOLO, &pcmMutePlay);
    setMute(mute);
    //第四步 创建缓冲区和回调函数    注册回调缓冲区 获取缓冲队列接口
    (*pcmPlayerObject)->GetInterface(pcmPlayerObject, SL_IID_BUFFERQUEUE, &pcmBufferQueue);
    setVolume(volumePercent);
    //缓冲接口回调
    (*pcmBufferQueue)->RegisterCallback(pcmBufferQueue, pcmBufferCallBack, this);
    //每次播完或录完一个 buffer 都会回调创建时 RegisterCallback 传入的callback方法，代表当前buffer数据已经处理完毕，buffer出队， 如果是录音你应该获取这个 buffer 保存下来，写文件或者其他操作都可以；如果是播放就代表 当前buffer已经播放完毕，你可以 Enqueue 下一个buffer了
    //
    //作者：WuTa0
    //链接：https://www.jianshu.com/p/5308ab38a3d0
    //来源：简书
    //著作权归作者所有。商业转载请联系作者获得授权，非商业转载请注明出处。



    //获取播放状态接口
    (*pcmPlayerPlay)->SetPlayState(pcmPlayerPlay, SL_PLAYSTATE_PLAYING);
    pcmBufferCallBack(pcmBufferQueue, this);
}

SLuint32 MyAudio::getCurrentSampleRateForOpensles(int sample_rate) {

    //#define SL_SAMPLINGRATE_8		((SLuint32) 8000000)
    //#define SL_SAMPLINGRATE_11_025	((SLuint32) 11025000)
    //#define SL_SAMPLINGRATE_12		((SLuint32) 12000000)
    //#define SL_SAMPLINGRATE_16		((SLuint32) 16000000)
    //#define SL_SAMPLINGRATE_22_05	((SLuint32) 22050000)
    //#define SL_SAMPLINGRATE_24		((SLuint32) 24000000)
    //#define SL_SAMPLINGRATE_32		((SLuint32) 32000000)
    //#define SL_SAMPLINGRATE_44_1	((SLuint32) 44100000)
    //#define SL_SAMPLINGRATE_48		((SLuint32) 48000000)
    //#define SL_SAMPLINGRATE_64		((SLuint32) 64000000)
    //#define SL_SAMPLINGRATE_88_2	((SLuint32) 88200000)
    //#define SL_SAMPLINGRATE_96		((SLuint32) 96000000)
    //#define SL_SAMPLINGRATE_192	((SLuint32) 192000000)
    switch (sample_rate) {
        case 8000:
            return SL_SAMPLINGRATE_8;
        case 11025:
            return SL_SAMPLINGRATE_11_025;
        case 12000:
            return SL_SAMPLINGRATE_12;
        case 16000:
            return SL_SAMPLINGRATE_16;
        case 22050:
            return SL_SAMPLINGRATE_22_05;
        case 24000:
            return SL_SAMPLINGRATE_24;
        case 32000:
            return SL_SAMPLINGRATE_32;
        case 44100:
            return SL_SAMPLINGRATE_44_1;
        case 48000:
            return SL_SAMPLINGRATE_48;
        case 64000:
            return SL_SAMPLINGRATE_64;
        case 88200:
            return SL_SAMPLINGRATE_88_2;
        case 96000:
            return SL_SAMPLINGRATE_96;
        case 192000:
            return SL_SAMPLINGRATE_192;
        default:
            return SL_SAMPLINGRATE_44_1;
    }
}
void MyAudio::pause() {
    if(pcmPlayerObject != NULL) {
        (*pcmPlayerPlay)->SetPlayState(pcmPlayerPlay, SL_PLAYSTATE_PAUSED);
    }
}

void MyAudio::resume() {
    if(pcmPlayerObject != NULL) {
        (*pcmPlayerPlay)->SetPlayState(pcmPlayerPlay, SL_PLAYSTATE_PLAYING);
    }
}

void MyAudio::stop() {
    if(pcmPlayerObject != NULL) {
        (*pcmPlayerPlay)->SetPlayState(pcmPlayerPlay, SL_PLAYSTATE_STOPPED);
    }
}

void MyAudio::release() {
    stop();

    if (bufferQueue != NULL) {
        bufferQueue->noticeThread();
        pthread_join(pcmCallBackThread, NULL); // 线程阻塞在pcmCallBackThread，直到pcmCallBackThread退出
        bufferQueue->release();
        delete(bufferQueue);
        bufferQueue = NULL;
    }

    if(queue != NULL) {
        delete(queue); // delete时 会走析构函数
        queue = NULL;
    }

    //释放 openSLES
    if (pcmPlayerObject != NULL) {
        (*pcmPlayerObject)->Destroy(pcmPlayerObject);
        pcmPlayerObject = NULL;
        pcmPlayerPlay = NULL;
        pcmBufferQueue = NULL;
        pcmMutePlay = NULL;
        pcmVolumePlay = NULL;
    }
    if (outputMixObject != NULL) {
        (*outputMixObject)->Destroy(outputMixObject);
        outputMixObject = NULL;
        outputMixEnvironmentalReverb = NULL;
    }
    if(engineObject != NULL) {
        (*engineObject)->Destroy(engineObject);
        engineObject = NULL;
        engineEngine = NULL;
    }

    if (buffer != NULL) {
        free(buffer);
        buffer = NULL;
    }

    if (out_buffer != NULL) {
        //申请out_buffer时候 用的是和buffer同一个内存，所以无需free
        out_buffer = NULL;
    }

    if (soundTouch != NULL) {
        delete soundTouch;
        soundTouch = NULL;
    }

    if (sampleBuffer != NULL) {
        free(sampleBuffer);
        sampleBuffer = NULL;
    }

    //解码器上下文释放
    if (avCodecContext != NULL) {
        avcodec_close(avCodecContext);
        avcodec_free_context(&avCodecContext);
        avCodecContext = NULL;
    }

    //只置空指针就行了 因为后面ffmepg还会用到
    if (playstatus != NULL) {
        playstatus = NULL;
    }
    if (callJava != NULL) {
        callJava = NULL;
    }
}

void MyAudio::setVolume(int percent) {
    volumePercent = percent;
    if(pcmVolumePlay != NULL)
    {
        if(percent > 30)
        {
            (*pcmVolumePlay)->SetVolumeLevel(pcmVolumePlay, (100 - percent) * -20);
        }
        else if(percent > 25)
        {
            (*pcmVolumePlay)->SetVolumeLevel(pcmVolumePlay, (100 - percent) * -22);
        }
        else if(percent > 20)
        {
            (*pcmVolumePlay)->SetVolumeLevel(pcmVolumePlay, (100 - percent) * -25);
        }
        else if(percent > 15)
        {
            (*pcmVolumePlay)->SetVolumeLevel(pcmVolumePlay, (100 - percent) * -28);
        }
        else if(percent > 10)
        {
            (*pcmVolumePlay)->SetVolumeLevel(pcmVolumePlay, (100 - percent) * -30);
        }
        else if(percent > 5)
        {
            (*pcmVolumePlay)->SetVolumeLevel(pcmVolumePlay, (100 - percent) * -34);
        }
        else if(percent > 3)
        {
            (*pcmVolumePlay)->SetVolumeLevel(pcmVolumePlay, (100 - percent) * -37);
        }
        else if(percent > 0)
        {
            (*pcmVolumePlay)->SetVolumeLevel(pcmVolumePlay, (100 - percent) * -40);
        }
        else{
            (*pcmVolumePlay)->SetVolumeLevel(pcmVolumePlay, (100 - percent) * -100);
        }
    }
}

void MyAudio::setMute(int mute) {
    this->mute = mute;
    LOGE("setMute-> %d", mute);
    if (pcmMutePlay != NULL) {
        if (mute == 0){
            //右声道
            LOGE("右声道");
            (*pcmMutePlay)->SetChannelMute(pcmMutePlay, 1, false);
            (*pcmMutePlay)->SetChannelMute(pcmMutePlay, 0, true);
        } else if (mute == 1) {
            //左声道
            LOGE("左声道");
            (*pcmMutePlay)->SetChannelMute(pcmMutePlay, 1, true);
            (*pcmMutePlay)->SetChannelMute(pcmMutePlay, 0, false);
        } else if (mute == 2) {
            //立体声
            (*pcmMutePlay)->SetChannelMute(pcmMutePlay, 1, false);
            (*pcmMutePlay)->SetChannelMute(pcmMutePlay, 0, false);
        }
    } else {
        LOGE("pcmMutePlay == NULL");
    }
}

void MyAudio::setPitch(float pitch) {
    this->pitch = pitch;
    if (soundTouch != NULL) {
        soundTouch->setPitch(pitch);
    }
}

void MyAudio::setSpeed(float speed) {
    this->speed = speed;
    if (soundTouch != NULL) {
        soundTouch->setTempo(speed);
    }
}

int MyAudio::getPCMDB(char *pcmdata, size_t pcmsize) {
    int db = 0; //db
    short int pervalue = 0; //每一帧的16位的值
    double sum = 0; //总的值 可能比较大
    for (int i = 0; i <pcmsize; i += 2) {
        memcpy(&pervalue, pcmdata + i, 2); //把每一次的值copy到pervalue里，每一次取2个char值 就是16位
        sum += abs(pervalue); //全部16位的和
    }
    sum = sum / (pcmsize / 2); //除以 多少个16位 求平均值
    if (sum > 0) {
        db = (int)20.0 * log10(sum); //求分贝公式
    }
    //一段时间的分贝
    return db;
}

void MyAudio::startStopRecord(bool start) {
    this->isRecordPcm = start;
}
