//
// Created by qiaopc on 2021/9/5.
//

#ifndef QIAOPCMUSIC_MYPCMBEAN_H
#define QIAOPCMUSIC_MYPCMBEAN_H

#include "SoundTouch.h"

using namespace soundtouch;

class MyPcmBean {

public:
    char *buffer; //缓冲
    int buffsize; //缓冲大小

public :
    MyPcmBean(SAMPLETYPE *buffer, int size);
    ~MyPcmBean();
};


#endif //QIAOPCMUSIC_MYPCMBEAN_H
