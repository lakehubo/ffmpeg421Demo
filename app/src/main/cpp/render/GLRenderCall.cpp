//
// Created by lake on 2020/4/15.
//

#include <LogUtil.h>
#include "GLRenderCall.h"

GLRenderCall::GLRenderCall() {

}

GLRenderCall::~GLRenderCall() {
    vm = nullptr;
    env = nullptr;
    requestRenderId = NULL;
}

void GLRenderCall::initCall(JavaVM *javaVm, JNIEnv *jniEnv, jobject obj) {
    vm = javaVm;
    env = jniEnv;
    jObj = env->NewGlobalRef(obj);
    jclass clz = env->GetObjectClass(obj);
    requestRenderId = env->GetMethodID(clz, "updateFrame", "()V");
}

void GLRenderCall::attachCall() {
    jint res = vm->GetEnv((void **) &env,JNI_VERSION_1_6);
    if (res != JNI_OK) {
        res = vm->AttachCurrentThread(&env, NULL);
        if (JNI_OK != res)
            LOGCATE("Failed to AttachCurrentThread, ErrorCode = %d", res);
    }
}

void GLRenderCall::callRequestRenderInThr() {
    env->CallVoidMethod(jObj,requestRenderId);
}

void GLRenderCall::detachCall() {
    vm->DetachCurrentThread();
}

void GLRenderCall::releaseCallObj(JNIEnv *env) {
    env->DeleteGlobalRef(jObj);
}
