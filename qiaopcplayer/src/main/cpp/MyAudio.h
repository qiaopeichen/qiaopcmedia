//
// Created by qiaopc on 2021/8/30.
//

#ifndef QIAOPCMUSIC_MYAUDIO_H
#define QIAOPCMUSIC_MYAUDIO_H

#include "MyQueue.h"
#include "Playstatus.h"
#include "CallJava.h"
#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>

#include "SoundTouch.h" //soundTouch库
#include "MyBufferQueue.h"
#include "MyPcmBean.h"

using namespace soundtouch;

extern "C" {
#include <libavcodec/avcodec.h>
#include <libswresample/swresample.h>
#include <libavutil/time.h>
};

class MyAudio {
public:
    int streamIndex = -1;
    AVCodecParameters *codecpar = NULL; // 包含音视频参数的结构体。很重要，可以用来获取音视频参数中的宽度、高度、采样率、编码格式等信息。
    AVCodecContext *avCodecContext = NULL; //关于编解码的结构体
    MyQueue *queue = NULL;
    Playstatus *playstatus = NULL;
    CallJava *callJava = NULL;

    pthread_t thread_play;
    AVPacket *avPacket = NULL;
    AVFrame *avFrame = NULL;
    int ret = -1;
    uint8_t *buffer = NULL;
    int data_size = NULL;
    int sample_rate = 0; //采样率

    int duration = 0; //播放时长
    AVRational time_base; //这个流的每一帧，持续的时间的分数表达式
    double now_time = 0;
    double clock = 0; //记录当前时间，递增
    double last_time = 0;
    int volumePercent = 100;
    int mute = 2;

    float pitch = 1.0f;
    float speed = 1.0f;

    bool isRecordPcm = false;
    bool readFrameFinish = true;

    pthread_t pcmCallBackThread;
    MyBufferQueue *bufferQueue;
    int defaultPcmSize = 4096;

    //引擎接口
    SLObjectItf engineObject = NULL;
    SLEngineItf engineEngine = NULL;

    //混音器
    SLObjectItf outputMixObject = NULL;
    SLEnvironmentalReverbItf outputMixEnvironmentalReverb = NULL;
    SLEnvironmentalReverbSettings reverbSettings = SL_I3DL2_ENVIRONMENT_PRESET_STONECORRIDOR; //石廊混音？

    //pcm
    SLObjectItf  pcmPlayerObject = NULL;
    SLPlayItf pcmPlayerPlay = NULL;
    SLVolumeItf pcmVolumePlay = NULL;
    SLMuteSoloItf  pcmMutePlay = NULL;
    //缓冲器队列接口
    SLAndroidSimpleBufferQueueItf  pcmBufferQueue = NULL;

    //SoundTouch
    SoundTouch *soundTouch = NULL;
    SAMPLETYPE *sampleBuffer = NULL;
    bool finished = true;
    uint8_t *out_buffer = NULL;
    // 源码：typedef	unsigned char		__uint8_t;
    //而 *_t是typedef定义的表示标志，是结构的一种标注。即我们所看到的 uint8_t、uint16_t、uint32_t都不是新的数据类型，而是通过typedef给类型起得别名。（如C语言中没有bool类型，有的程序员用int表示，有的用short表示，则利用统一的定义来表示bool，是比较好的。typedef char bool）。
    //
    //
    //  则很明显的看出：uint8_t是用1个字节表示的；uint16_t是用2个字节表示的；uint32_t是用4个字节表示的。
    // uint8_t实际上就是一个char，所以输出 uint8_t类型的变量实际上输出对应的字符
    //————————————————
    //版权声明：本文为CSDN博主「时光机ﾟ」的原创文章，遵循CC 4.0 BY-SA版权协议，转载请附上原文出处链接及本声明。
    //原文链接：https://blog.csdn.net/qq_19784349/article/details/82927169
    int nb = 0;
    int num = 0;//返回的采样个数

    bool isCut = false;
    int end_time = 0;
    bool showPcm = false;

public:
    MyAudio(Playstatus *playstatus, int sample_rate, CallJava *callJava);
    ~MyAudio();

    void play();
    int resampleAudio(void **pcmbuf);
    void initOpenSLES();
    SLuint32 getCurrentSampleRateForOpensles(int sample_rate);

    void pause();
    void resume();

    void stop();
    void release();

    void setVolume(int percent);

    void setMute(int mute);

    int getSoundTouchData();

    void setPitch(float pitch);

    void setSpeed(float speed);

    //  与int固定四个字节不同有所不同,size_t的取值range是目标平台下最大可能的数组尺寸,
    //  一些平台下size_t的范围小于int的正数范围,又或者大于unsigned int. 使用Int既有可能浪费，又有可能范围不够大。
    int getPCMDB(char *pcmdata, size_t pcmsize);//获取分贝

    void startStopRecord(bool start);
};


#endif //QIAOPCMUSIC_MYAUDIO_H
