//
// Created by lake on 2020/4/15.
//

#ifndef FFMPEG421DEMO_GLRENDERCALL_H
#define FFMPEG421DEMO_GLRENDERCALL_H


#include <jni.h>

class GLRenderCall {

public:
    JavaVM *vm;
    JNIEnv *env;
    jobject jObj;
    jmethodID requestRenderId;

public:
    GLRenderCall();
    ~GLRenderCall();

    void initCall(JavaVM *javaVm,JNIEnv *env, jobject obj);
    void attachCall();
    void callRequestRenderInThr();
    void detachCall();
    void releaseCallObj(JNIEnv *env);
};


#endif //FFMPEG421DEMO_GLRENDERCALL_H
