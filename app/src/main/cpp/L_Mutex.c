//
// Created by lake on 2020/3/12.
//
#include <malloc.h>
#include "L_Mutex.h"

/**
 * 创建锁
 * @return
 */
pthread_mutex_t *createMutex(void) {
    pthread_mutex_t *mutex;
    mutex = calloc(1, sizeof(pthread_mutex_t));
    pthread_mutexattr_t attr;
    if (mutex) {
        if (pthread_mutex_init(mutex, &attr) != 0) {
            free(mutex);
            mutex = NULL;
        }
    } else {
        free(mutex);
    }
    return mutex;
}

/**
 * 销毁锁
 * @param mutex
 */
void destroyMutex(pthread_mutex_t *mutex) {
    if (mutex) {
        pthread_mutex_destroy(mutex);
        free(mutex);
    }
}

/**
 * 上锁
 * @param mutex
 * @return
 */
int lockMutex(pthread_mutex_t *mutex) {
    if (mutex == NULL) {
        return -1;
    }
    if (pthread_mutex_lock(mutex) != 0) {
        return -1;
    }
    return 0;
}

/**
 * 解锁
 * @param mutex
 * @return
 */
int unlockMutex(pthread_mutex_t *mutex) {
    if (mutex == NULL) {
        return -1;
    }
    if (pthread_mutex_unlock(mutex) != 0) {
        return -1;
    }
    return 0;
}