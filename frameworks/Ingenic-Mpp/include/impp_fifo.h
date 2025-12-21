#ifndef __IMPP_FIFO_H__
#define __IMPP_FIFO_H__

#include <pthread.h>
#include <semaphore.h>

struct impp_fifo {
        int maxElem;
        int Tail;
        int Head;
        int ElemNum;
        void **ElemBuffer;
        pthread_mutex_t mutex;
        pthread_cond_t cond;
        sem_t count_sem;
};

typedef struct impp_fifo impp_fifo_t;


int impp_fifo_init(impp_fifo_t *fifo, int elem_num);

void impp_fifo_deinit(impp_fifo_t *fifo);

int impp_fifo_queue(impp_fifo_t *fifo, void *elem, unsigned int wait);

void *impp_fifo_dequeue(impp_fifo_t *fifo, unsigned int wait);

int impp_fifo_getMaxElem(impp_fifo_t *fifo);

int impp_fifo_getElemNum(impp_fifo_t *fifo);

#endif // __IMPP_FIFO_H__
