//
// Created by qiaopc on 2021/8/30.
//

#ifndef QIAOPCMUSIC_MYFFMEPG_H
#define QIAOPCMUSIC_MYFFMEPG_H

#include "CallJava.h"
#include "pthread.h"
#include "MyAudio.h"
#include "MyVideo.h"

extern "C"
{
#include "libavformat/avformat.h"
#include <libavutil/time.h>
};

class MyFFmepg {
public:
    CallJava *callJava = NULL;
    const char *url = NULL;
    pthread_t decodeThread;
    AVFormatContext *pFormatCtx = NULL;
    MyAudio *audio = NULL;
    MyVideo *video = NULL;
    Playstatus *playstatus = NULL;

    pthread_mutex_t init_mutex;
    bool exit = false;
    int duration = 0;
    pthread_mutex_t seek_mutex;
public:
    MyFFmepg(Playstatus *playstatus, CallJava *callJava, const char* url);
    ~MyFFmepg();
    void prepared();
    void decodeFFmpegThread();

    void start();

    void pause();
    void resume();
    void release();

    void seek(int64_t secds);
    void setVolume(int percent);

    void setMute(int mute);
    void setPitch(float pitch);
    void setSpeed(float speed);
    int getSampleRate();

    void startStopRecord(bool start);

    bool cutAudioPlay(int start_time, int end_time, bool showPcm);

    int getCodecContext( AVCodecParameters *codecpar, AVCodecContext **avCodecContext);
};


#endif //QIAOPCMUSIC_MYFFMEPG_H
