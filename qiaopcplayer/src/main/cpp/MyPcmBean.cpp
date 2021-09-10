//
// Created by qiaopc on 2021/9/5.
// 分包之后的大小类
//

#include "MyPcmBean.h"

MyPcmBean::MyPcmBean(SAMPLETYPE *buffer, int size) { // SAMPLETYPE soundtouch中的 16bit short
    this->buffer = static_cast<char *>(malloc(size)); //声明内存空间
    this->buffsize = size; //
    memcpy(this->buffer, buffer, size); //buffer拷贝到this->buffer 中
}

MyPcmBean::~MyPcmBean() {
    free(buffer);
    buffer = NULL;
}
