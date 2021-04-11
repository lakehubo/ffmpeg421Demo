//
// Created by lake on 2020/4/2.
//
#include <string.h>
#include <malloc.h>
#include <libavutil/mem.h>
#include <LogUtil.h>
#include "L_Callback.h"
#include "L_Mutex.h"

int createCallback(CallbackContext *callback, JavaVM *vm, JNIEnv *env, jobject obj) {
    memset(callback, 0, sizeof(callback));
    callback->java_vm = vm;
    callback->pid = 0;
    callback->env = env;
    callback->callbackObj = obj;
    jclass clz = (*env)->GetObjectClass(callback->env, callback->callbackObj);
    callback->progressId = (*callback->env)->GetMethodID(callback->env, clz, "onProgress", "(D)V");
    callback->videoInfoId = (*callback->env)->GetMethodID(callback->env, clz, "videoInfo", "(D)V");
    return 0;
}

int attachCallback(CallbackContext *callback) {
    jint res = (*callback->java_vm)->GetEnv(callback->java_vm, (void **) &callback->env,
                                            JNI_VERSION_1_6);
    if (res != JNI_OK) {
        res = (*callback->java_vm)->AttachCurrentThread(callback->java_vm, &callback->env, NULL);
        if (JNI_OK != res) {
            LOGCATE("Failed to AttachCurrentThread, ErrorCode = %d", res);
            return -1;
        }
    }
    callback->pid = 1;
    return 0;
}

void callbackProgress(CallbackContext *callback, double pos) {
    (*callback->env)->CallVoidMethod(callback->env,
                                     callback->callbackObj,
                                     callback->progressId,
                                     pos);
}

void callbackVideoInfo(CallbackContext *callback, double time) {
    (*callback->env)->CallVoidMethod(callback->env,
                                     callback->callbackObj,
                                     callback->videoInfoId,
                                     time);
}

void detachCallback(CallbackContext *callback) {
    callback->env = NULL;
    callback->pid = 0;
    (*callback->java_vm)->DetachCurrentThread(callback->java_vm);
}

void deleteCallback(CallbackContext *callback, JNIEnv *env) {
    (*env)->DeleteGlobalRef(env, callback->callbackObj);
}