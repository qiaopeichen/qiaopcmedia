//
// Created by ywl on 2017-12-3.
//

#ifndef WLPLAYER_BUFFERQUEUE_H
#define WLPLAYER_BUFFERQUEUE_H

#include "deque"
#include "Playstatus.h"
#include "MyPcmBean.h"

extern "C"
{
#include <libavcodec/avcodec.h>
#include "pthread.h"
};

class MyBufferQueue {

public:
    std::deque<MyPcmBean *> queueBuffer;
    pthread_mutex_t mutexBuffer;
    pthread_cond_t condBuffer;
    Playstatus *wlPlayStatus = NULL;

public:
    MyBufferQueue(Playstatus *playstatus);
    ~MyBufferQueue();
    int putBuffer(SAMPLETYPE *buffer, int size);
    int getBuffer(MyPcmBean **pcmBean);
    int clearBuffer();

    void release();
    int getBufferSize();

    int noticeThread();
};


#endif //WLPLAYER_BUFFERQUEUE_H
