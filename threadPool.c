//
// Noam Schwartz
// 200160042
//

#include <pthread.h>
#include <unistd.h>
#include <fcntl.h>
#include "threadPool.h"

#define ERROR "Error in system call\n"

typedef struct task{
    void (*computeFunc)(void *param);
    void* param;
}Task;

/*
 * This function runs the tasks on the threadpool threads.
 */
void* run(void* threadPool){
        //cast threadpool from void.
        ThreadPool *tp = (ThreadPool*) threadPool;
        OSQueue* taskQ = tp->queue;
        //while the threadpool is still running and isnt asked to close and there are still tasks in the queue.
        while((tp->isShuttingDown == 0) && !(tp->canAddMore == 0 && osIsQueueEmpty(taskQ))){
            if (pthread_mutex_lock(&(tp->lock))!= 0 ) write(2, ERROR, sizeof(ERROR));
            //if there are still tasks/
            if ((tp->isShuttingDown == 0) && (osIsQueueEmpty(taskQ) == 1)){
                if ( pthread_cond_wait(&(tp->update), &(tp->lock)) != 0){
                    write(2, ERROR, sizeof(ERROR));
                }
            }
            //if there are no tasks in the queue.
            if (osIsQueueEmpty(taskQ) == 1 ){
                if (pthread_mutex_unlock(&(tp->lock))!=0) write(2, ERROR, sizeof(ERROR));
            }
            else{
                //take task out of queue and carry out.
                Task *tsk = osDequeue(taskQ);
                pthread_mutex_unlock(&(tp->lock));
                tsk->computeFunc(tsk->param);
                free(tsk);
            }
        }
    return NULL;
}
/*
 * This function creates a threadpool with N threads.
 * it returns the new created threadpool
 */
ThreadPool* tpCreate(int numOfThreads){
    ThreadPool *tp;
    //allocate memory for pool
    if ((tp = (ThreadPool*)malloc(sizeof(ThreadPool))) == NULL){
        write(2, ERROR, sizeof(ERROR));
        return NULL;
    }
    tp->threadsNum = numOfThreads;
    //allocate memory for N threads.
    if ((tp->threads = (pthread_t*)malloc(sizeof(pthread_t) * tp->threadsNum)) == NULL){
        write(2, ERROR, sizeof(ERROR));
        return NULL;
    }
    if (pthread_mutex_init(&(tp->lock), NULL)!=0) write(2, ERROR, sizeof(ERROR));
    //create queue and update struct.
    tp->queue = osCreateQueue();
    tp->canAddMore = 1;
    tp->isShuttingDown=0;
    if ((pthread_mutex_init(&(tp->lock), NULL) != 0)){
        write(2, ERROR, sizeof(ERROR));
        tpDestroy(tp, 0);
        return NULL;
    }
    if (pthread_cond_init(&(tp->update), NULL) != 0){
        write(2, ERROR, sizeof(ERROR));
        tpDestroy(tp, 0);
        return NULL;
    }
    //create N threads.
    int i;
    for (i = 0; i<numOfThreads; i++){
        if (pthread_create(&(tp->threads[i]), NULL, run, (void*)tp) !=0){
            //if creation failed
            write(2, ERROR, sizeof(ERROR));
            tpDestroy(tp, 0);
            return NULL;
        }
    }
    return tp;
}
/*
 * This function inserts a task into the queue.
 * it returns 0 if succeded and -1 if it failed.
 */
int tpInsertTask(ThreadPool* threadPool, void (*computeFunc) (void *), void* param){
    if ((threadPool == NULL) || ((threadPool->canAddMore == 0) || (computeFunc == NULL))){
        return -1;
    }
    if (pthread_mutex_lock(&(threadPool->lock))!=0) write(2, ERROR, sizeof(ERROR));
    Task* task;
    //allocate memory for the task.
    if ((task = (Task*)malloc(sizeof(Task))) == NULL){
        write(2, ERROR, sizeof(ERROR));
        return -1;
    }
    task->computeFunc = computeFunc;
    task->param = param;
    // add task to the queue.
    osEnqueue(threadPool->queue, (void*) task);
    if (pthread_cond_signal(&(threadPool->update)) != 0){
        //error
        write(2, ERROR, sizeof(ERROR));
        return 1;
    }
    if (pthread_mutex_unlock((&(threadPool->lock)))!=0) write(2, ERROR, sizeof(ERROR));
    return 0;
}
/*
 * This function destroys the threadpool.
 */
void tpDestroy(ThreadPool* threadPool, int shouldWaitForTasks){
    //change canAddMore boolian
    if (threadPool->canAddMore == 1){
        if (pthread_mutex_lock(&(threadPool->lock))!= 0 ) write(2, ERROR, sizeof(ERROR));
        threadPool->canAddMore = 0;
        if (pthread_mutex_unlock(&(threadPool->lock))!=0 ) write(2, ERROR, sizeof(ERROR));
    }
    else{
        return;
    }
    if ((pthread_cond_broadcast(&(threadPool->update)) != 0)){
        write(2, ERROR, sizeof(ERROR));
        return;
    }
    //join for all threads.
    int i;
    for (i = 0; i < threadPool->threadsNum; i++){
        pthread_join(threadPool->threads[i], NULL);
    }
    //if we dont want to wait for the rest of the tasks to be carried out.
    if (shouldWaitForTasks == 0){
        if (pthread_mutex_lock(&(threadPool->lock))!=0) write(2, ERROR, sizeof(ERROR));
        while (!(osIsQueueEmpty(threadPool->queue))){
            Task* task = osDequeue(threadPool->queue);
            free(task);
        }
        if (pthread_mutex_unlock(&(threadPool->lock))!=0) write(2, ERROR, sizeof(ERROR));
    }
    //free all the rest of the queue and pool.
    osDestroyQueue(threadPool->queue);
    free(threadPool->threads);
    if (pthread_mutex_destroy(&(threadPool->lock))!=0) write(2, ERROR, sizeof(ERROR));
    free(threadPool);
}


