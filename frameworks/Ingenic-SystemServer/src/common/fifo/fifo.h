#ifndef __FIFO_H__
#define __FIFO_H__

#include <pthread.h>
#include <semaphore.h>

struct fifo {
        int maxElem;
        int Tail;
        int Head;
        int ElemNum;
        void **ElemBuffer;
        pthread_mutex_t mutex;
        pthread_cond_t cond;
        sem_t count_sem;
};

typedef struct fifo fifo_t;

/* wait type */
#define NO_WAIT            0                                       /*!< 非阻塞等待   */
#define WAIT_FOREVER   0xFFFFFFFF                  /*!< 阻塞等待     */


int init_fifo(fifo_t *fifo, int elem_num);

void deinit_fifo(fifo_t *fifo);

int queue_fifo(fifo_t *fifo, void *elem, unsigned int wait);

void *dequeue_fifo(fifo_t *fifo, unsigned int wait);

int getFifoMaxElem(fifo_t *fifo);

int getFifoElemNum(fifo_t *fifo);

#endif // __FIFO_H__
