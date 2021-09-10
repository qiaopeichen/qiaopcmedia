//
// Created by qiaopc on 2021/8/31.
//

#ifndef QIAOPCMUSIC_MYQUEUE_H
#define QIAOPCMUSIC_MYQUEUE_H

#include "queue"
#include "pthread.h"
#include "AndroidLog.h"
#include "Playstatus.h"
extern "C"{
#include "libavcodec/avcodec.h"
};

class MyQueue {

public:
    std::queue<AVPacket *> queuePacket;
    pthread_mutex_t mutexPacket;
    pthread_cond_t condPacket;
    Playstatus *playstatus = NULL;

public:
    MyQueue(Playstatus *playstatus);
    ~MyQueue();

    int putAvpacket(AVPacket *packet);
    int getAvpacket(AVPacket *packet);
    int getQueueSize();
    void clearAvpacket();
};


#endif //QIAOPCMUSIC_MYQUEUE_H
