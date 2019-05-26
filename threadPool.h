//
// Noam Schwartz
// 200160042
//

#ifndef OS4_THREADPOOL_H
#define OS4_THREADPOOL_H


#include <sys/param.h>
#include "osqueue.h"
#include "malloc.h"

typedef struct thread_pool
{
    pthread_mutex_t lock;
    pthread_cond_t update;
    pthread_t *threads;
    OSQueue *queue;
    int threadsNum;
    int isShuttingDown;
    int canAddMore;
}ThreadPool;

ThreadPool* tpCreate(int numOfThreads);

void tpDestroy(ThreadPool* threadPool, int shouldWaitForTasks);

int tpInsertTask(ThreadPool* threadPool, void (*computeFunc) (void *), void* param);


#endif //OS4_THREADPOOL_H
