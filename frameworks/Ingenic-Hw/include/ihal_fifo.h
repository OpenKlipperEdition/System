#ifndef __IHAL_FIFO_H__
#define __IHAL_FIFO_H__

#include <pthread.h>
#include <semaphore.h>

struct ihal_fifo {
        int maxElem;
        int Tail;
        int Head;
        int ElemNum;
        void **ElemBuffer;
        pthread_mutex_t mutex;
        pthread_cond_t cond;
        sem_t count_sem;
};

typedef struct ihal_fifo ihal_fifo_t;


int ihal_fifo_init(ihal_fifo_t *fifo, int elem_num);

void ihal_fifo_deinit(ihal_fifo_t *fifo);

int ihal_fifo_queue(ihal_fifo_t *fifo, void *elem, unsigned int wait);

void *ihal_fifo_dequeue(ihal_fifo_t *fifo, unsigned int wait);

int ihal_fifo_getMaxElem(ihal_fifo_t *fifo);

int ihal_fifo_getElemNum(ihal_fifo_t *fifo);

#endif // __IHAL_FIFO_H__
