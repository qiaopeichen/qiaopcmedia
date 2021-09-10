//
// Created by qiaopc on 2021/8/31.
//

#include "MyQueue.h"

MyQueue::MyQueue(Playstatus *playstatus) {
    this->playstatus = playstatus;
    pthread_mutex_init(&mutexPacket, NULL);//初始化线程锁
    pthread_cond_init(&condPacket, NULL); //初始化条件变量
}

MyQueue::~MyQueue() {
    clearAvpacket();
    pthread_mutex_destroy(&mutexPacket);
    pthread_cond_destroy(&condPacket);
}

int MyQueue::putAvpacket(AVPacket *packet) {
    pthread_mutex_lock(&mutexPacket);
    queuePacket.push(packet);
//    if(LOG_DEBUG) {
//        LOGD("放入一个AVpacket到队列里面，个数为：%d", queuePacket.size());
//    }
    pthread_cond_signal(&condPacket);//发送信息
    pthread_mutex_unlock(&mutexPacket);
    return 0;
}

int MyQueue::getAvpacket(AVPacket *packet) {
    pthread_mutex_lock(&mutexPacket);

    while (playstatus != NULL && !playstatus->exit) {
        if (queuePacket.size() > 0) {
            AVPacket *avPacket = queuePacket.front();
            if (av_packet_ref(packet, avPacket) == 0) {//把avPacket拷贝到packet中
                queuePacket.pop();
            }
            av_packet_free(&avPacket);
            av_free(avPacket);
            avPacket = NULL;
//            if (LOG_DEBUG) {
//                LOGD("从队列里取出一个AVpacket，还剩下 %d 个", queuePacket.size());
//            }
            break;
        } else {
            pthread_cond_wait(&condPacket, &mutexPacket); //线程等待，释放锁
        }
    }
    pthread_mutex_unlock(&mutexPacket);
    return 0;
}

int MyQueue::getQueueSize() {
    int size = 0;
    pthread_mutex_lock(&mutexPacket);
    size = queuePacket.size();
    pthread_mutex_unlock(&mutexPacket);
    return size;
}

void MyQueue::clearAvpacket() {
    pthread_cond_signal(&condPacket);//发送信号量
    pthread_mutex_lock(&mutexPacket);

    while (!queuePacket.empty()) {
        AVPacket *packet = queuePacket.front();
        queuePacket.pop();
        av_packet_free(&packet);
        av_free(packet);
        packet = NULL;
    }

    pthread_mutex_unlock(&mutexPacket);
}
