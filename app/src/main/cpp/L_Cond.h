//
// Created by lake on 2020/3/12.
//

#ifndef FFMPEG421DEMO_L_COND_H
#define FFMPEG421DEMO_L_COND_H

#include <pthread.h>
#include <unistd.h>

pthread_cond_t *createCond(void);

void destroyCond(pthread_cond_t *cond);

int condSignal(pthread_cond_t *cond);

int waitTimeout(pthread_cond_t *cond, pthread_mutex_t *mutex, uint32_t ms);

int condWait(pthread_cond_t *cond, pthread_mutex_t *mutex);

#endif //FFMPEG421DEMO_L_COND_H
