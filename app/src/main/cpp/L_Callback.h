//
// Created by lake on 2020/4/2.
//

#ifndef FFMPEG421DEMO_L_CALLBACK_H
#define FFMPEG421DEMO_L_CALLBACK_H

#include <jni.h>
#include <sys/types.h>

typedef struct CallbackContext {
    JavaVM *java_vm;
    JNIEnv *env;
    jobject callbackObj;//回调函数对象
    jmethodID progressId;//进度回调
    jmethodID videoInfoId;//视频信息回调
    int pid;//当前线程id
    pthread_mutex_t mutex;
} CallbackContext;

int createCallback(CallbackContext *callback, JavaVM *vm, JNIEnv *env, jobject obj);
int attachCallback(CallbackContext *callback);
void callbackProgress(CallbackContext *callback, double pos);
void callbackVideoInfo(CallbackContext *callback, double time);
void detachCallback(CallbackContext *callback);
void deleteCallback(CallbackContext *callback, JNIEnv *env);

#endif //FFMPEG421DEMO_L_CALLBACK_H
