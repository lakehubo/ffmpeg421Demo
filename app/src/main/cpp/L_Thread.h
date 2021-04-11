//
// Created by lake on 2020/3/12.
//

#ifndef FFMPEG421DEMO_L_THREAD_H
#define FFMPEG421DEMO_L_THREAD_H
#include <pthread.h>

pthread_t *createThread(void *(*fn)(void *), void *arg);

pthread_t *createDetachThread(void*(*fn)(void*), void *args);

void waitThread(pthread_t thread);

void detachThread(pthread_t thread);

#endif //FFMPEG421DEMO_L_THREAD_H
