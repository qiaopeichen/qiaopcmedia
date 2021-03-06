//
// Created by qiaopc on 2021/9/7.
//

#ifndef QIAOPCMUSIC_MYVIDEO_H
#define QIAOPCMUSIC_MYVIDEO_H


#include "MyQueue.h"
#include "CallJava.h"
#include "MyAudio.h"
#define CODEC_YUV 0 //软解码
#define CODEC_MEDIACODEC 1 //硬解码
extern "C" {
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
#include "include/libavcodec/avcodec.h"
#include <libavutil/time.h>

};

class MyVideo {
public:
    int streamIndex = -1; //流的索引
    AVCodecContext *avCodecContext = NULL;// 解码器上下文
    AVCodecParameters *codecpar = NULL;
    MyQueue *queue = NULL;
    Playstatus *playstatus = NULL;
    CallJava *callJava = NULL;
    AVRational time_base; //这个流的每一帧，持续的时间的分数表达式

    pthread_t thread_play;

    MyAudio *audio = NULL;
    double clock = 0;
    double delayTime = 0;
    double defaultDelayTime = 0.04; //默认帧数25
    pthread_mutex_t codecMutex;
    int codectype = CODEC_YUV;

    AVBSFContext *abs_ctx = NULL;
public:
    MyVideo(Playstatus *playstatus, CallJava *callJava);
    ~MyVideo();
    void play();
    void release();

    double getFrameDiffTime(AVFrame * avFrame, AVPacket *avPacket);
    double getDelayTime(double diff);
};


#endif //QIAOPCMUSIC_MYVIDEO_H
