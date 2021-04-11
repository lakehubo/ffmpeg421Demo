//
// Created by lake on 2020/3/12.
//
#include <libavcodec/avcodec.h>
#include "L_Thread.h"

/**
 * 创建线程
 * @param fn 线程方法
 * @param args 线程参数
 * @return
 */
pthread_t *createThread(void *(*fn)(void *), void *args) {
    pthread_t *thread;
    thread = calloc(1, sizeof(*thread));
    if (!thread) {
        return NULL;
    }
    pthread_attr_t type;
    if (pthread_attr_init(&type) != 0) {
        free(thread);
        return NULL;
    }
    pthread_attr_setdetachstate(&type, PTHREAD_CREATE_JOINABLE);

    /* Create the thread and go! */
    if (pthread_create(thread, &type, fn, args) != 0) {
        free(thread);
        return NULL;
    }
    return thread;
}

pthread_t *createDetachThread(void *(*fn)(void *), void *args){
    pthread_t *thread;
    thread = calloc(1, sizeof(*thread));
    if (!thread) {
        return NULL;
    }
    pthread_attr_t type;
    if (pthread_attr_init(&type) != 0) {
        free(thread);
        return NULL;
    }
    pthread_attr_setdetachstate(&type, PTHREAD_CREATE_DETACHED);

    /* Create the thread and go! */
    if (pthread_create(thread, &type, fn, args) != 0) {
        free(thread);
        return NULL;
    }
    return thread;
}

void waitThread(pthread_t thread) {
    pthread_join(thread, 0);
}

void detachThread(pthread_t thread) {
    pthread_detach(thread);
}