#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "fifo.h"



#define TAG     "fifo"

int init_fifo(fifo_t *fifo, int elem_num)
{
        fifo->maxElem = elem_num + 1;
        fifo->Tail = 0;
        fifo->Head = 0;
        fifo->ElemNum = 0;
        fifo->ElemBuffer = (void **)malloc(fifo->maxElem * sizeof(void *));
        if (!fifo->ElemBuffer) {
                return -1;
        }
        pthread_mutex_init(&fifo->mutex, NULL);
        pthread_cond_init(&fifo->cond, NULL);
        sem_init(&fifo->count_sem, 0, elem_num);

        return 0;
}

void deinit_fifo(fifo_t *fifo)
{
        free(fifo->ElemBuffer);
        pthread_mutex_destroy(&fifo->mutex);
        pthread_cond_destroy(&fifo->cond);
        sem_destroy(&fifo->count_sem);
}

int queue_fifo(fifo_t *fifo, void *elem, unsigned int wait)
{
        int ret = 0;
        if (wait == WAIT_FOREVER) {
                sem_wait(&fifo->count_sem);
        } else {
                ret = sem_trywait(&fifo->count_sem);
                if (ret) {
                        return -1;
                }
        }
        pthread_mutex_lock(&fifo->mutex);
        fifo->ElemBuffer[fifo->Tail] = elem;
        fifo->Tail = (fifo->Tail + 1) % fifo->maxElem;
        ++fifo->ElemNum;
        pthread_cond_signal(&fifo->cond);
        pthread_mutex_unlock(&fifo->mutex);

        return 0;
}

void *dequeue_fifo(fifo_t *fifo, unsigned int wait)
{
        void *pElem;
        pthread_mutex_lock(&fifo->mutex);
        while (wait == WAIT_FOREVER) {
                if (fifo->ElemNum > 0) {
                        break;
                }
                if (wait == WAIT_FOREVER) {
                        pthread_cond_wait(&fifo->cond, &fifo->mutex);
                }
        }
        if (fifo->ElemNum > 0) {
                pElem = fifo->ElemBuffer[fifo->Head];
                fifo->Head = (fifo->Head + 1) % fifo->maxElem;
                fifo->ElemNum--;
                sem_post(&fifo->count_sem);
        } else {
                pthread_mutex_unlock(&fifo->mutex);
                return NULL;
        }
        pthread_mutex_unlock(&fifo->mutex);
        return pElem;
}

int getFifoMaxElem(fifo_t *fifo)
{
        pthread_mutex_lock(&fifo->mutex);
        int ret = fifo->maxElem;
        pthread_mutex_unlock(&fifo->mutex);
        return ret;
}

int getFifoElemNum(fifo_t *fifo)
{
        int ret = 0;
        pthread_mutex_lock(&fifo->mutex);
        ret = fifo->ElemNum;
        pthread_mutex_unlock(&fifo->mutex);
        return ret;
}

