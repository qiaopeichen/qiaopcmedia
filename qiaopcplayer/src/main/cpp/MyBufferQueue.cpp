//
// Created by ywl on 2017-12-3.
//

#include "MyBufferQueue.h"
#include "AndroidLog.h"

MyBufferQueue::MyBufferQueue(Playstatus *playStatus) {
    wlPlayStatus = playStatus;
    pthread_mutex_init(&mutexBuffer, NULL);
    pthread_cond_init(&condBuffer, NULL);
}


MyBufferQueue::~MyBufferQueue() {
    wlPlayStatus = NULL;
    pthread_mutex_destroy(&mutexBuffer);
    pthread_cond_destroy(&condBuffer);
    if(LOG_DEBUG)
    {
        LOGE("WlBufferQueue 释放完了");
    }
}

void MyBufferQueue::release() {

    if(LOG_DEBUG)
    {
        LOGE("WlBufferQueue::release");
    }
    noticeThread();
    clearBuffer();

    if(LOG_DEBUG)
    {
        LOGE("WlBufferQueue::release success");
    }
}

int MyBufferQueue::putBuffer(SAMPLETYPE *buffer, int size) {
    pthread_mutex_lock(&mutexBuffer);
    MyPcmBean *pcmBean = new MyPcmBean(buffer, size);
    queueBuffer.push_back(pcmBean);
    pthread_cond_signal(&condBuffer);
    pthread_mutex_unlock(&mutexBuffer);
    return 0;
}

int MyBufferQueue::getBuffer(MyPcmBean **pcmBean) {

    pthread_mutex_lock(&mutexBuffer);

    while(wlPlayStatus != NULL && !wlPlayStatus->exit)
    {
        if(queueBuffer.size() > 0)
        {
            *pcmBean = queueBuffer.front();
            queueBuffer.pop_front();
            break;
        } else{
            if(!wlPlayStatus->exit)
            {
                pthread_cond_wait(&condBuffer, &mutexBuffer);
            }
        }
    }
    pthread_mutex_unlock(&mutexBuffer);
    return 0;
}

int MyBufferQueue::clearBuffer() {

    pthread_cond_signal(&condBuffer);
    pthread_mutex_lock(&mutexBuffer);
    while (!queueBuffer.empty())
    {
        MyPcmBean *pcmBean = queueBuffer.front();
        queueBuffer.pop_front();
        delete(pcmBean);
    }
    pthread_mutex_unlock(&mutexBuffer);
    return 0;
}

int MyBufferQueue::getBufferSize() {
    int size = 0;
    pthread_mutex_lock(&mutexBuffer);
    size = queueBuffer.size();
    pthread_mutex_unlock(&mutexBuffer);
    return size;
}


int MyBufferQueue::noticeThread() {
    pthread_cond_signal(&condBuffer);
    return 0;
}



