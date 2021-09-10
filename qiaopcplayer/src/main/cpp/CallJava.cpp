//
// Created by qiaopc on 2021/8/29.
//这个类用于 c++ 调用 java方法
//
#include <jni.h>
#include "CallJava.h"
#include "AndroidLog.h"


CallJava::CallJava(JavaVM *javaVM, JNIEnv *env, jobject *obj) {
    this->javaVM = javaVM;
    this->jniEnv = env;
    this->jobj = env->NewGlobalRef(*obj);//NewGlobalRef：创建全局引用，传入一个局部引用参数 , 将局部引用转为全局引用

    jclass jlz = jniEnv->GetObjectClass(jobj); //获取Java层对应的类
    if(!jlz) {
        if (LOG_DEBUG) {
            LOGE("get jclass wrong");
        }
        return;
    }

    jmid_prepared = env->GetMethodID(jlz, "onCallPrepared", "()V");//获取Java层被回调的函数
    jmid_load = env->GetMethodID(jlz, "onCallLoad", "(Z)V");
    jmid_timeinfo = env->GetMethodID(jlz, "onCallTimeInfo", "(II)V");
    jmid_error = env->GetMethodID(jlz, "onCallError", "(ILjava/lang/String;)V");
    jmid_complete = env->GetMethodID(jlz, "onCallComplete", "()V");
    jmid_valumedb = env->GetMethodID(jlz, "onCallValumeDB", "(I)V");
    jmid_callpcmtoacc = env->GetMethodID(jlz, "encodecPcmToAAC", "(I[B)V");
    jmid_pcminfo = env->GetMethodID(jlz, "onCallPcmInfo", "([BI)V");
    jmid_pcmrate = env->GetMethodID(jlz, "onCallPcmRate", "(I)V");
}

void CallJava::onCallPrepared(int type) {
    if (type == MAIN_THREAD) {
        jniEnv->CallVoidMethod(jobj, jmid_prepared);
    } else if (type == CHILD_THREAD) {
        JNIEnv *jniEnv;
        if (javaVM->AttachCurrentThread(&jniEnv, 0) != JNI_OK) { //从全局的JavaVM中获取到环境变量，获取到当前线程中的JNIEnv指针
            if (LOG_DEBUG) {
                LOGE("get child thread jnienv wrong");
            }
            return;
        }
        jniEnv->CallVoidMethod(jobj, jmid_prepared);
        javaVM->DetachCurrentThread();
    }
}

void CallJava::onCallLoad(int type, bool load) {
    if (type == MAIN_THREAD) {
        jniEnv->CallVoidMethod(jobj, jmid_load, load);
    } else if (type == CHILD_THREAD) {
        JNIEnv *jniEnv;
        if (javaVM->AttachCurrentThread(&jniEnv, 0) != JNI_OK) { //从全局的JavaVM中获取到环境变量，获取到当前线程中的JNIEnv指针
            if (LOG_DEBUG) {
                LOGE("get child thread jnienv wrong");
            }
            return;
        }
        jniEnv->CallVoidMethod(jobj, jmid_load, load);
        javaVM->DetachCurrentThread();
    }
}

void CallJava::onCallTimeInfo(int type, int curr, int total) {
    if (type == MAIN_THREAD) {
        jniEnv->CallVoidMethod(jobj, jmid_timeinfo, curr, total);
    } else if (type == CHILD_THREAD) {
        JNIEnv *jniEnv;
        if (javaVM->AttachCurrentThread(&jniEnv, 0) != JNI_OK) { //从AttachCurrentThread全局的JavaVM中获取到环境变量，获取到当前线程中的JNIEnv指针
            if (LOG_DEBUG) {
                LOGE("get child thread jnienv wrong");
            }
            return;
        }
        jniEnv->CallVoidMethod(jobj, jmid_timeinfo, curr, total);
        javaVM->DetachCurrentThread();
    }
}

CallJava::~CallJava() {

}

void CallJava::CallError(int type, int code, char *msg) {
    if (type == MAIN_THREAD) {
        jstring jmsg = jniEnv->NewStringUTF(msg);
        jniEnv->CallVoidMethod(jobj, jmid_error, code, jmsg);
        jniEnv->DeleteLocalRef(jmsg); //释放内存
    } else if (type == CHILD_THREAD) {
        JNIEnv *jniEnv;
        int status;
        bool isAttached = false;
        status = javaVM->GetEnv((void**)&jniEnv, JNI_VERSION_1_4);
        if (status < 0) {
            if (javaVM->AttachCurrentThread(&jniEnv, 0) != JNI_OK) { //从全局的JavaVM中获取到环境变量，获取到当前线程中的JNIEnv指针
                if (LOG_DEBUG) {
                    LOGE("get child thread jnienv wrong");
                }
                return;
            }
            isAttached = true;
        }

        jstring jmsg = jniEnv->NewStringUTF(msg);
        jniEnv->CallVoidMethod(jobj, jmid_error, code, jmsg);
        jniEnv->DeleteLocalRef(jmsg); //释放内存

        if (isAttached){
            //调用DetachCurrentThread函数的地方在java线程中，
            // 即在java调用C++代码时在C++代码中调用了AttachCurrentThread方法来获取JNIEnv，
            // 此时JNIEnv已经通过参数传递进来，你不需要再次AttachCurrentThread来获取。在释放时就会报错。
            javaVM->DetachCurrentThread();
        }
    }
}

void CallJava::onCallComplete(int type) {
    if (type == MAIN_THREAD) {
        jniEnv->CallVoidMethod(jobj, jmid_complete);
    } else if (type == CHILD_THREAD) {
        JNIEnv *jniEnv;
        int status;
        bool isAttached = false;
        status = javaVM->GetEnv((void**)&jniEnv, JNI_VERSION_1_4);
        if (status < 0) {
            if (javaVM->AttachCurrentThread(&jniEnv, 0) != JNI_OK) { //从全局的JavaVM中获取到环境变量，获取到当前线程中的JNIEnv指针
                if (LOG_DEBUG) {
                    LOGE("get child thread jnienv wrong");
                }
                return;
            }
            isAttached = true;
        }
        jniEnv->CallVoidMethod(jobj, jmid_complete);
        if (isAttached){
            javaVM->DetachCurrentThread();
        }
    }
}

void CallJava::onCallValumeDB(int type, int db) {
    if (type == MAIN_THREAD) {
        jniEnv->CallVoidMethod(jobj, jmid_valumedb, db);
    } else if (type == CHILD_THREAD) {
        JNIEnv *jniEnv;
        int status;
        bool isAttached = false;
        status = javaVM->GetEnv((void**)&jniEnv, JNI_VERSION_1_4);
        if (status < 0) {
            if (javaVM->AttachCurrentThread(&jniEnv, 0) != JNI_OK) { //从全局的JavaVM中获取到环境变量，获取到当前线程中的JNIEnv指针
                if (LOG_DEBUG) {
                    LOGE("get child thread jnienv wrong");
                }
                return;
            }
            isAttached = true;
        }
        jniEnv->CallVoidMethod(jobj, jmid_valumedb, db);
        if (isAttached){
            javaVM->DetachCurrentThread();
        }
    }
}

void CallJava::onCallPcmToAAc(int type, int size, void *buffer) {
    if (type == MAIN_THREAD) {
        jbyteArray jbuffer = jniEnv->NewByteArray(size);
        jniEnv->SetByteArrayRegion(jbuffer, 0, size, static_cast<const jbyte *>(buffer));
        jniEnv->CallVoidMethod(jobj, jmid_callpcmtoacc, size, jbuffer);
        jniEnv->DeleteLocalRef(jbuffer);
    } else if (type == CHILD_THREAD) {
        JNIEnv *jniEnv;
        int status;
        bool isAttached = false;
        status = javaVM->GetEnv((void**)&jniEnv, JNI_VERSION_1_4);
        if (status < 0) {
            if (javaVM->AttachCurrentThread(&jniEnv, 0) != JNI_OK) { //从全局的JavaVM中获取到环境变量，获取到当前线程中的JNIEnv指针
                if (LOG_DEBUG) {
                    LOGE("get child thread jnienv wrong");
                }
                return;
            }
            isAttached = true;
        }
        jbyteArray jbuffer = jniEnv->NewByteArray(size);
        jniEnv->SetByteArrayRegion(jbuffer, 0, size, static_cast<const jbyte *>(buffer));
        jniEnv->CallVoidMethod(jobj, jmid_callpcmtoacc, size, jbuffer);
        jniEnv->DeleteLocalRef(jbuffer);
        if (isAttached){
            javaVM->DetachCurrentThread();
        }
    }
}

void CallJava::onCallPcmInfo(void *buffer, int size) {

    JNIEnv *jniEnv;
    int status;
    bool isAttached = false;
    status = javaVM->GetEnv((void **) &jniEnv, JNI_VERSION_1_4);
    if (status < 0) {
        if (javaVM->AttachCurrentThread(&jniEnv, 0) !=
            JNI_OK) { //从全局的JavaVM中获取到环境变量，获取到当前线程中的JNIEnv指针
            if (LOG_DEBUG) {
                LOGE("get child thread jnienv wrong");
            }
            return;
        }
        isAttached = true;
    }
    jbyteArray jbuffer = jniEnv->NewByteArray(size);
    jniEnv->SetByteArrayRegion(jbuffer, 0, size, static_cast<const jbyte *>(buffer));
    jniEnv->CallVoidMethod(jobj, jmid_pcminfo,jbuffer, size);
    jniEnv->DeleteLocalRef(jbuffer);
    if (isAttached) {
        javaVM->DetachCurrentThread();
    }
}

void CallJava::onCallPcmRate(int samplerate) {
    JNIEnv *jniEnv;
    int status;
    bool isAttached = false;
    status = javaVM->GetEnv((void **) &jniEnv, JNI_VERSION_1_4);
    if (status < 0) {
        if (javaVM->AttachCurrentThread(&jniEnv, 0) !=
            JNI_OK) { //从全局的JavaVM中获取到环境变量，获取到当前线程中的JNIEnv指针
            if (LOG_DEBUG) {
                LOGE("get child thread jnienv wrong");
            }
            return;
        }
        isAttached = true;
    }
    jniEnv->CallVoidMethod(jobj, jmid_pcmrate,samplerate);
    if (isAttached) {
        javaVM->DetachCurrentThread();
    }
}
