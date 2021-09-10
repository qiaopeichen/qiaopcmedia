//
// Created by qiaopc on 2021/8/30.
//

#include "MyFFmepg.h"
MyFFmepg::MyFFmepg(Playstatus *playstatus, CallJava *callJava, const char *url) {
    this->callJava = callJava;
    this->url = url;
    this->playstatus = playstatus;
    pthread_mutex_init(&init_mutex, NULL);
    pthread_mutex_init(&seek_mutex, NULL);
}

void *decodeFFmpeg(void *data) {
    MyFFmepg *myFFmepg = (MyFFmepg *) data;
    myFFmepg->decodeFFmpegThread();
    pthread_exit(&myFFmepg->decodeThread);
}

void MyFFmepg::prepared() {

    /**
     * int pthread_create(
                 pthread_t *restrict tidp,   //新创建的线程ID指向的内存单元。
                 const pthread_attr_t *restrict attr,  //线程属性，默认为NULL
                 void *(*start_rtn)(void *), //新创建的线程从start_rtn函数的地址开始运行
                 void *restrict arg //默认为NULL。若上述函数需要参数，将参数放入结构中并将地址作为arg传入。
                  );
     */
    pthread_create(&decodeThread, NULL, decodeFFmpeg, this);//它的功能是创建线程（实际上就是确定调用该线程函数的入口点），在线程创建以后，就开始运行相关的线程函数。
}

int avformat_callback(void *ctx) {
    MyFFmepg *fFmepg = static_cast<MyFFmepg *>(ctx);
    if (fFmepg->playstatus->exit) {
        return AVERROR_EOF; //end of file 达到文件末尾，结束执行
    }
    return 0;
}

void MyFFmepg::decodeFFmpegThread() {
    pthread_mutex_lock(&init_mutex);
//    av_register_all(); // 注册解码器 ffmpeg新版本已经弃用，不必初始化
    avformat_network_init(); //网络初始化
    pFormatCtx = avformat_alloc_context(); // 上下文初始化
    //设置超时回调,有些不存在的地址或者网络不好的情况下要卡很久
    pFormatCtx->interrupt_callback.callback = avformat_callback; //callback
    pFormatCtx->interrupt_callback.opaque = this; // callback的参数

    if (avformat_open_input(&pFormatCtx, url, NULL, NULL) != 0) { //该函数用于打开多媒体数据并且获得一些相关的信息。它的声明位于libavformat\avformat.h
        if (LOG_DEBUG) {
            LOGE("can not open url: %s", url);
            callJava->CallError(CHILD_THREAD, 1001,"can not open url");
        }
        exit = true;
        pthread_mutex_unlock(&init_mutex);
        return;
    }
    if (avformat_find_stream_info(pFormatCtx, NULL) < 0) {
        //avformat_find_stream_info主要是读一些包（packets ），然后从中提取初流的信息。有一些文件格式没有头，比如说MPEG格式的，
        //这个时候，这个函数就很有用，因为它可以从读取到的包中获得到流的信息。在MPEG-2重复帧模式的情况下，该函数还计算真实的帧率。
        //逻辑文件位置不被此函数更改; 读出来的包会被缓存起来供以后处理。
        if (LOG_DEBUG) {
            LOGE("can not find streams from url: %s", url);
            callJava->CallError(CHILD_THREAD, 1002,"can not find streams from url");
        }
        exit = true;
        pthread_mutex_unlock(&init_mutex);
        return;
    }
    for (int i = 0; i < pFormatCtx->nb_streams; i++) { //pFormatCtx->streams 是一个 AVStream 指针的数组，里面包含了媒体资源的每一路流信息，数组的大小为 pFormatCtx->nb_streams
        if (pFormatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) { //得到音频流
            if (audio == NULL) {
                audio = new MyAudio(playstatus, pFormatCtx->streams[i]->codecpar->sample_rate, callJava);
                audio->streamIndex = i; //设置索引
                audio->codecpar = pFormatCtx->streams[i]->codecpar;
                audio->duration = pFormatCtx->duration / AV_TIME_BASE;//时间基，理解为时间单位
                audio->time_base = pFormatCtx->streams[i]->time_base;
                duration = audio->duration;
                callJava->onCallPcmRate(audio->sample_rate);
            }
        } else if (pFormatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {//得到视频流
            if (video == NULL) {
                video = new MyVideo(playstatus, callJava);
                video->streamIndex = i;
                video->codecpar = pFormatCtx->streams[i]->codecpar;
                video->time_base = pFormatCtx->streams[i]->time_base;
            }
        }
    }

    if (audio != NULL) {
        getCodecContext(audio->codecpar, &audio->avCodecContext);
    }
    if (video != NULL) {
        getCodecContext(video->codecpar, &video->avCodecContext);
    }

    //回调java层
    if(callJava != NULL)
    {
        if(playstatus != NULL && !playstatus->exit)
        {
            callJava->onCallPrepared(CHILD_THREAD);
        } else{
            exit = true;
        }
    }
    pthread_mutex_unlock(&init_mutex);
}

//回调java层onprepared之后，开始start
void MyFFmepg::start() {
    if (audio == NULL) {
        if (LOG_DEBUG) {
            LOGE("audio is null");
            callJava->CallError(CHILD_THREAD, 1007,"audio is null");
        }
        return;
    }

    audio->play();
    video->play();
    int count = 0;
    while (playstatus != NULL && !playstatus->exit) {

        if (playstatus->seek) {
            av_usleep(100 *1000); //睡眠100毫秒
            continue;
        }
        if (audio->queue->getQueueSize() > 100) {
            continue;
        }
        AVPacket *avPacket = av_packet_alloc(); //AVPacket是存储压缩编码数据相关信息的结构体，一个avpacket里，可能存在多个frame
        pthread_mutex_lock(&seek_mutex);
        int ret = av_read_frame(pFormatCtx, avPacket); //
        pthread_mutex_unlock(&seek_mutex);

        if (ret == 0) {//av_read_frame()获取视频的一帧，不存在半帧说法。
            //            av_read_frame
            //            返回流的下一帧。
            //            *此函数返回存储在文件中的内容，但不验证解码器是否有有效帧。
            //            它将把文件中存储的内容拆分为帧，并为每个调用返回一个帧。
            //            它不会省略有效帧之间的无效数据，以便给解码器最大可能的解码信息。
            //            如果pkt->buf为NULL，那么直到下一个av_read_frame()或直到avformat_close_input()，包都是有效的。
            //            否则数据包将无限期有效。在这两种情况下，当不再需要包时，必须使用av_free_packet释放包。
            //            对于视频，数据包只包含一帧。
            //            对于音频，如果每个帧具有已知的固定大小(例如PCM或ADPCM数据)，则它包含整数帧数。
            //            如果音频帧有一个可变的大小(例如MPEG音频)，那么它包含一帧。
            //            在AVStream中，pkt->pts、pkt->dts和pkt->持续时间总是被设置为恰当的值。
            //            time_base单元(猜测格式是否不能提供它们)。
            //            如果视频格式为B-frames，pkt->pts可以是AV_NOPTS_VALUE，所以如果不解压缩有效负载，最好依赖pkt->dts。
            if (avPacket->stream_index == audio->streamIndex) {
                count++;
                LOGE("解码第%d 帧", count);
                //stream_index：标识该AVPacket所属的视频/音频流。
                audio->queue->putAvpacket(avPacket);
            } else if (avPacket->stream_index == video->streamIndex) {
                video->queue->putAvpacket(avPacket);
            } else {
                av_packet_free(&avPacket);
                av_free(avPacket);
                avPacket = NULL;
            }
        } else {
            av_packet_free(&avPacket);
            av_free(avPacket);
            avPacket = NULL;
            while (playstatus != NULL && !playstatus->exit) {
                if (audio->queue->getQueueSize() > 0) {
                    av_usleep(100 *1000); //睡眠100毫秒
                    continue;
                } else {
                    playstatus->exit = true;
                    break;
                }
            }
        }
    }
    if (callJava != NULL) {
        callJava->onCallComplete(CHILD_THREAD);
    }
    exit = true;
    if (LOG_DEBUG) {
        LOGD("解码完成");
    }
}

void MyFFmepg::pause() {
    if (audio != NULL) {
        audio->pause();
    }
}

void MyFFmepg::resume() {
    if (audio != NULL) {
        audio->resume();
    }
}

void MyFFmepg::release() {
    if (LOG_DEBUG) {
        LOGE("开始释放ffmpeg");
    }
//    if (playstatus->exit) {
//        return;
//    }
    playstatus->exit = true;
    pthread_mutex_lock(&init_mutex);

    int sleepCount = 0;
    while (!exit) {
        if (sleepCount > 1000) {
            exit = true;
        }
        if(LOG_DEBUG) {
            LOGE("wait ffmpeg exit %d", sleepCount);
            callJava->CallError(CHILD_THREAD, 1008,"wait ffmpeg exit");
        }
        sleepCount++;
        av_usleep(10 * 1000);//睡眠10毫秒
    }

    if (LOG_DEBUG) {
        LOGE("释放audio");
    }
    if (audio != NULL) {
        audio->release();
        delete(audio);
        audio = NULL;
    }
    if (LOG_DEBUG) {
        LOGE("释放video");
    }
    if (video != NULL) {
        video->release();
        delete(video);
        video = NULL;
    }
    if (LOG_DEBUG) {
        LOGE("释放pFormatCtx");
    }
    if (pFormatCtx != NULL) {
        avformat_close_input(&pFormatCtx);
        avformat_free_context(pFormatCtx);
        pFormatCtx = NULL;
    }
    if (LOG_DEBUG) {
        LOGE("释放playstatus");
    }
    if (playstatus != NULL) {
        playstatus = NULL;
    }
    if (LOG_DEBUG) {
        LOGE("释放callJava");
    }
    if (callJava != NULL) {
        callJava = NULL;
    }

    pthread_mutex_unlock(&init_mutex);
}

MyFFmepg::~MyFFmepg() {
    pthread_mutex_destroy(&init_mutex);
    pthread_mutex_destroy(&seek_mutex);
}

void MyFFmepg::seek(int64_t secds) {
    if (duration <= 0) {
        return;
    }
    if(secds >= 0 && secds <= duration) {
        if (audio != NULL) {
            playstatus->seek = true;
            audio->queue->clearAvpacket();
            audio->clock = 0;
            audio->last_time = 0;
            pthread_mutex_lock(&seek_mutex);

            int64_t rel = secds * AV_TIME_BASE; //真实时间 = 秒数 * 时间基
            avcodec_flush_buffers(audio->avCodecContext);
            avformat_seek_file(pFormatCtx, -1, INT64_MIN, rel, INT64_MAX, 0); // -1为全部音频文件
            pthread_mutex_unlock(&seek_mutex);
            playstatus->seek = false;
        }
    }
}

void MyFFmepg::setVolume(int percent) {
    if (audio != NULL) {
        audio->setVolume(percent);
    }
}

void MyFFmepg::setMute(int mute) {
    if (audio != NULL) {
        audio->setMute(mute);
    }
}

void MyFFmepg::setPitch(float pitch) {
    if (audio != NULL) {
        audio->setPitch(pitch);
    }
}

void MyFFmepg::setSpeed(float speed) {
    if (audio != NULL) {
        audio->setSpeed(speed);
    }
}

int MyFFmepg::getSampleRate() {
    if (audio != NULL) {
        return audio->avCodecContext->sample_rate;
    }
    return 0;
}

void MyFFmepg::startStopRecord(bool start) {
    if (audio != NULL) {
        audio->startStopRecord(start);
    }
}

bool MyFFmepg::cutAudioPlay(int start_time, int end_time, bool showPcm) {
    if (start_time >= 0 && end_time <= duration && start_time < end_time) {
        audio->isCut = true;
        audio->end_time = end_time;
        audio->showPcm = showPcm;
        seek(start_time);
        return true;
    }
    return false;
}

int MyFFmepg::getCodecContext(AVCodecParameters *codecpar, AVCodecContext **avCodecContext) {//**指向指针的地址
    AVCodec *dec = avcodec_find_decoder(codecpar->codec_id);//该方法用于寻找解码器；codec_id：编码数据类型
    if (!dec) {
        if (LOG_DEBUG) {
            LOGE("can not find decoder");
        }
        callJava->CallError(CHILD_THREAD, 1003,"can not find decoder");
        exit = true;
        pthread_mutex_unlock(&init_mutex);
        return -1;
    }
    *avCodecContext/*指向指针的值*/ = avcodec_alloc_context3(dec);//分配解码器
    if (!*avCodecContext) {
        if (LOG_DEBUG) {
            LOGE("can not alloc new decoderctx");
            callJava->CallError(CHILD_THREAD, 1004,"can not alloc new decoderctx");
        }
        exit = true;
        pthread_mutex_unlock(&init_mutex);
        return -1;
    }

    //把解码器信息复制到解码器上下文中
    if (avcodec_parameters_to_context(*avCodecContext, codecpar) < 0) {
        if (LOG_DEBUG) {
            LOGE("can not find decoderctx");
            callJava->CallError(CHILD_THREAD, 1005,"can not find decoderctx");
        }
        exit = true;
        pthread_mutex_unlock(&init_mutex);
        return -1;
    }
    if (avcodec_open2(*avCodecContext, dec, 0) != 0) { //avcodec_open2 该函数用于初始化一个视音频编解码器的AVCodecContext
        if (LOG_DEBUG) {
            LOGE("can not open audio streams");
            callJava->CallError(CHILD_THREAD, 1006,"can not open audio streams");
        }
        exit = true;
        pthread_mutex_unlock(&init_mutex);
        return -1;
    }
    return 0;
}
