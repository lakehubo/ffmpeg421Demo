//
// Created by lake on 2020/3/12.
//
#include <malloc.h>
#include <asm/errno.h>
#include "L_Cond.h"

/**
 * 创建信号量
 * @return
 */
pthread_cond_t *createCond(void) {
    pthread_cond_t *cond;
    cond = calloc(1, sizeof(pthread_cond_t));
    if (cond) {
        if (pthread_cond_init(cond, NULL) != 0) {
            free(cond);
            cond = NULL;
        }
    }
    return cond;
}

/**
 * 销毁信号量
 * @param cond
 */
void destroyCond(pthread_cond_t *cond) {
    if (cond) {
        pthread_cond_destroy(cond);
        free(cond);
    }
}

/**
 * 发送信号量
 * @param cond
 * @return
 */
int condSignal(pthread_cond_t *cond) {
    int retval;
    if (!cond) {
        return -1;
    }
    retval = 0;
    if (pthread_cond_signal(cond) != 0) {
        return -1;
    }
    return retval;
}

/**
 * 等待信号 超时
 * @param cond
 * @param mutex
 * @param ms
 * @return
 */
int waitTimeout(pthread_cond_t *cond, pthread_mutex_t *mutex, uint32_t ms) {
    int retval;
    struct timeval delta;
    struct timespec abstime;
    if (!cond) {
        return -1;
    }
    clock_gettime(CLOCK_REALTIME, &abstime);
    abstime.tv_nsec += (ms % 1000) * 1000000;
    abstime.tv_sec += ms / 1000;
    if (abstime.tv_nsec > 1000000000) {
        abstime.tv_sec += 1;
        abstime.tv_nsec -= 1000000000;
    }
    tryagain:
    retval = pthread_cond_timedwait(cond, mutex, &abstime);
    switch (retval) {
        case EINTR:
            goto tryagain;
            /* break; -Wunreachable-code-break */
        case ETIMEDOUT:
            retval = 110;
            break;
        case 0:
            break;
        default:
            retval = -1;
    }
    return retval;
}

/**
 * 等待信号
 * @param cond
 * @param mutex
 * @return
 */
int condWait(pthread_cond_t *cond, pthread_mutex_t *mutex) {
    if (!cond) {
        return -1;
    } else if (pthread_cond_wait(cond, mutex) != 0) {
        return -1;
    }
    return 0;
}