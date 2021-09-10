//
// Created by qiaopc on 2021/8/29.
//

#ifndef QIAOPCMUSIC_ANDROIDLOG_H
#define QIAOPCMUSIC_ANDROIDLOG_H

#endif //QIAOPCMUSIC_ANDROIDLOG_H

#include "android/log.h"

#define LOG_DEBUG true

#define LOGD(FORMAT,...) __android_log_print(ANDROID_LOG_DEBUG,"qiaopc",FORMAT,##__VA_ARGS__);
#define LOGE(FORMAT,...) __android_log_print(ANDROID_LOG_ERROR,"qiaopc",FORMAT,##__VA_ARGS__);