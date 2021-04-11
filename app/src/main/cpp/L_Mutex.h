//
// Created by lake on 2020/3/12.
//
/**
 * 线程锁 参照SDL_mutex代码实现
 */
#ifndef FFMPEG421DEMO_L_MUTEX_H
#define FFMPEG421DEMO_L_MUTEX_H
#include <pthread.h>

pthread_mutex_t *createMutex();

void destroyMutex(pthread_mutex_t *mutex);

int lockMutex(pthread_mutex_t *mutex);

int unlockMutex(pthread_mutex_t *mutex);

#endif //FFMPEG421DEMO_L_MUTEX_H


