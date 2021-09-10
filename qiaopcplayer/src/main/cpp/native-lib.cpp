#include <jni.h>
#include <string>
#include "CallJava.h"
#include "MyFFmepg.h"

extern "C"
{
#include <libavformat/avformat.h>
}

JavaVM *javaVM = NULL;
CallJava *callJava = NULL;
MyFFmepg *fFmepg = NULL;
Playstatus *playstatus = NULL;
pthread_t thread_start;
bool nexit = true;

extern "C"
JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *vm, void *reserved) {// 在加载动态链接库的时候，JVM会调用JNI_OnLoad(JavaVM* jvm, void* reserved)（如果定义了该函数）。
    jint result = -1;
    javaVM = vm; //JavaVM,英文全称是Java virtual machine，就是Java虚拟机。
    JNIEnv *env; //JNIEnv,英文全称是Java Native Interface Environment，就是Java本地接口环境。JNIEnv只在当前线程中有效。本地方法不能将JNIEnv从一个线程传递到另一个线程中
    if (vm->GetEnv((void **)&env, JNI_VERSION_1_4) != JNI_OK) {
        return result;
    }
    return JNI_VERSION_1_4;
}

extern "C"
JNIEXPORT void JNICALL
Java_com_example_qiaopcplayer_player_QiaopcPlayer_n_1prepared(JNIEnv *env, jobject instance,
                                                              jstring source_) {
    const char *source = env->GetStringUTFChars(source_, 0);

    // TODO
    if (fFmepg == NULL) {
        if (callJava == NULL) {
            callJava = new CallJava(javaVM, env, &instance);
        }
        callJava->onCallLoad(MAIN_THREAD, true);
        playstatus = new Playstatus();
        fFmepg = new MyFFmepg(playstatus, callJava, source); // 把Java层被回调的函数传入FFmepg类中
        fFmepg->prepared();
    }

//    env->ReleaseStringUTFChars(source_, source);
}

void *startCallBack(void *data) {
    MyFFmepg *fFmepg = static_cast<MyFFmepg *>(data);
    fFmepg->start();
    pthread_exit(&thread_start);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_example_qiaopcplayer_player_QiaopcPlayer_n_1start(JNIEnv *env, jobject instance) {

    if (fFmepg != NULL) {
        pthread_create(&thread_start, NULL, startCallBack,fFmepg);
    }
}extern "C"
JNIEXPORT void JNICALL
Java_com_example_qiaopcplayer_player_QiaopcPlayer_n_1pause(JNIEnv *env, jobject instance) {
    if(fFmepg != NULL) {
        fFmepg->pause();
    }
}extern "C"
JNIEXPORT void JNICALL
Java_com_example_qiaopcplayer_player_QiaopcPlayer_n_1resume(JNIEnv *env, jobject instance) {
    if(fFmepg != NULL) {
        fFmepg->resume();
    }
}extern "C"
JNIEXPORT void JNICALL
Java_com_example_qiaopcplayer_player_QiaopcPlayer_n_1stop(JNIEnv *env, jobject instance) {

    if (!nexit) {
        return;
    }
    nexit = false;

    jclass jlz = env->GetObjectClass(instance);
    jmethodID jmid_next = env->GetMethodID(jlz, "onCallNext", "()V");

    if (fFmepg != NULL) {
        fFmepg->release();
        delete(fFmepg);
        fFmepg = NULL;
        if (callJava != NULL) {
            delete(callJava);
            callJava = NULL;
        }
        if (playstatus != NULL) {
            delete(playstatus);
            playstatus = NULL;
        }
    }
    nexit = true;
    env->CallVoidMethod(instance, jmid_next);
}extern "C"
JNIEXPORT void JNICALL
Java_com_example_qiaopcplayer_player_QiaopcPlayer_n_1seek(JNIEnv *env, jobject instance,
                                                          jint secds) {
    if (fFmepg != NULL) {
        fFmepg->seek(secds);
    }

}
extern "C"
JNIEXPORT jint JNICALL
Java_com_example_qiaopcplayer_player_QiaopcPlayer_n_1duration(JNIEnv *env, jobject instance) {
    if(fFmepg != NULL) {
        return fFmepg->duration;
    }
    return 0;
}extern "C"
JNIEXPORT void JNICALL
Java_com_example_qiaopcplayer_player_QiaopcPlayer_n_1volume(JNIEnv *env, jobject instance,
                                                            jint percent) {

    if (fFmepg != NULL) {
        fFmepg->setVolume(percent);
    }
}extern "C"
JNIEXPORT void JNICALL
Java_com_example_qiaopcplayer_player_QiaopcPlayer_n_1mute(JNIEnv *env, jobject instance,
                                                          jint mute) {
    if (fFmepg != NULL) {
        fFmepg->setMute(mute);
    }
}extern "C"
JNIEXPORT void JNICALL
Java_com_example_qiaopcplayer_player_QiaopcPlayer_n_1pitch(JNIEnv *env, jobject instance,
                                                           jfloat pitch) {
    if (fFmepg != NULL) {
        fFmepg->setPitch(pitch);
    }
}extern "C"
JNIEXPORT void JNICALL
Java_com_example_qiaopcplayer_player_QiaopcPlayer_n_1speed(JNIEnv *env, jobject instance,
                                                           jfloat speed) {
    if (fFmepg != NULL) {
        fFmepg->setSpeed(speed);
    }
}extern "C"
JNIEXPORT jint JNICALL
Java_com_example_qiaopcplayer_player_QiaopcPlayer_n_1samplerate(JNIEnv *env, jobject instance) {
    if (fFmepg != NULL) {
       return fFmepg->getSampleRate();
    }
    return 0;
}extern "C"
JNIEXPORT void JNICALL
Java_com_example_qiaopcplayer_player_QiaopcPlayer_n_1startstoprecord(JNIEnv *env, jobject instance,
                                                                     jboolean start) {
    if (fFmepg != NULL) {
        fFmepg->startStopRecord(start);
    }
}extern "C"
JNIEXPORT jboolean JNICALL
Java_com_example_qiaopcplayer_player_QiaopcPlayer_n_1cuteaudioplay(JNIEnv *env, jobject instance,
                                                                   jint start_time, jint end_time,
                                                                   jboolean showPcm) {
    if (fFmepg != NULL) {
        return fFmepg->cutAudioPlay(start_time, end_time, showPcm);
    }
    return false;
}