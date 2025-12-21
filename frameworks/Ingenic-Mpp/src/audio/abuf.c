#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include "abuf.h"

#define ALIGN(d,a) (((d)+((a)-1))/(a)*(a))

struct buf_list_node;

struct buf_list {
        uint32_t num;
        uint32_t data_size;
        uint32_t info_size;
        struct buf_list_node *head;
        struct buf_list_node *tail;
};
struct buf_list_node {
        int idx;
        struct buf_list *pblist;
        struct buf_list_node *next;
        void *info;
        void *data;
};

void *buf_list_alloc(uint32_t num, uint32_t data_size, uint32_t info_size)
{
        uint32_t size = 0;
        uint32_t lsize = 0;
        uint32_t nsize = 0;
        uint32_t nstart = 0;
        uint32_t isize = 0;
        uint32_t istart = 0;
        uint32_t dsize = 0;
        uint32_t dstart = 0;
        uint32_t i = 0;
        void *pbuf = NULL;
        struct buf_list *pblist = NULL;
        struct buf_list_node *pn = NULL;
        struct buf_list_node *pn_next = NULL;

        lsize = ALIGN(sizeof(struct buf_list), 4);
        size = lsize;
        nsize = ALIGN(sizeof(struct buf_list_node), 4);
        nstart = size;
        size += nsize * num;
        isize = ALIGN(info_size, 4);
        istart = size;
        size += isize * num;
        dsize = ALIGN(data_size, 4);
        dstart = size;
        size += dsize * num;
        pbuf = malloc(size);
        if (NULL == pbuf) {
                printf("err: (%s:%d)malloc err %s\n", __FILE__, __LINE__, strerror(errno));
                return NULL;
        }
        memset(pbuf, 0, size);
        pblist = pbuf;
        pblist->num = num;
        pblist->data_size = data_size;
        pblist->info_size = info_size;
        if (0 == num) {
                return pblist;
        }
        pn = pbuf + nstart;
        pn->pblist = pblist;
        pn->info = pbuf + istart;
        pn->data = pbuf + dstart;
        pn->idx = 0;
        pblist->head = pn;
        for (i = 1; i < num; i++) {
                pn_next = pbuf + nstart + (nsize * i);
                pn_next->pblist = pblist;
                pn_next->info = pbuf + istart + (isize * i);
                pn_next->data = pbuf + dstart + (dsize * i);
                pn_next->next = NULL;
                pn_next->idx = i;
                pn->next = pn_next;
                pn = pn_next;
        }
        pblist->tail = pn;
        return pblist;
}

void buf_list_free(void *blist)
{
        if (NULL != blist) {
                free(blist);
        } else {
                printf("warn: (%s:%d)blist == NULL\n", __FILE__, __LINE__);
        }
}

void *buf_list_get_node(void *blist)
{
        struct buf_list_node *n;
        struct buf_list *bl;
        bl = blist;
        if (NULL == bl) {
                printf("err: (%s:%d)blist == NULL\n", __FILE__, __LINE__);
                return NULL;
        }
        if (bl->num <= 0) {
                return NULL;
        } else if (bl->num == 1) {
                n = bl->head;
                bl->head = NULL;
                bl->tail = NULL;
                bl->num = 0;
                return n;
        } else {
                n = bl->head;
                //      printf("info: bl->head = %p, bl->head->next = %p\n", bl->head, bl->head->next);
                bl->head = n->next;
                bl->num--;
                return n;
        }
}

void *buf_list_try_get_node(void *blist)
{
        struct buf_list_node *n;
        struct buf_list *bl;
        bl = blist;
        if (NULL == bl) {
                printf("err: (%s:%d)blist == NULL\n", __FILE__, __LINE__);
                return NULL;
        }
        if (bl->num <= 0) {
                return NULL;
        } else {
                n = bl->head;
                return n;
        }
}

void buf_list_put_node(void *blist, void *node)
{
        struct buf_list_node *n = node;
        struct buf_list *bl = blist;
        if (NULL == n) {
                printf("err: (%s:%d)n == NULL\n", __FILE__, __LINE__);
                return;
        }
        if (NULL == bl) {
                printf("err: (%s:%d)bl == NULL\n", __FILE__, __LINE__);
                return;
        }
        n->next = NULL;
        n->pblist = blist;
        if (bl->num <= 0) {
                bl->tail = n;
                bl->head = n;
                bl->num = 1;
        } else {
                bl->tail->next = n;
                bl->tail = n;
                bl->num++;
        }
}

uint32_t buf_list_get_num(void *blist)
{
        struct buf_list *bl;
        bl = blist;
        if (NULL == bl) {
                printf("err: (%s:%d)blist == NULL\n", __FILE__, __LINE__);
                return 0;
        }
        return bl->num;
}

void *buf_list_node_get_info(void *node)
{

        struct buf_list_node *n;
        n = node;
        if (NULL == n) {
                printf("err: (%s:%d)node == NULL\n", __FILE__, __LINE__);
                return NULL;
        }
        return n->info;
}

void *buf_list_node_get_data(void *node)
{

        struct buf_list_node *n;
        n = node;
        if (NULL == n) {
                printf("err: (%s:%d)node == NULL\n", __FILE__, __LINE__);
                return NULL;
        }
        return n->data;
}

int buf_list_node_index(void *node)
{

        struct buf_list_node *n;
        n = node;
        if (NULL == n) {
                printf("err: (%s:%d)node == NULL\n", __FILE__, __LINE__);
                return -1;
        }
        return n->idx;
}

struct audio_buf {
        struct buf_list *empty_list;
        struct buf_list *full_list;
};

void *audio_buf_alloc(uint32_t num, uint32_t data_size, uint32_t info_size)
{
        void *empty = NULL;
        void *full = NULL;
        struct audio_buf *ab = malloc(sizeof(struct audio_buf));
        if (NULL == ab) {
                printf("err: (%s:%d)malloc err %s\n", __FILE__, __LINE__, strerror(errno));
                return NULL;
        }
        empty = buf_list_alloc(num, data_size, info_size);
        if (NULL == empty) {
                printf("err: (%s:%d)malloc err %s\n", __FILE__, __LINE__, strerror(errno));
                return NULL;
        }
        full = buf_list_alloc(0, data_size, info_size);
        if (NULL == full) {
                printf("err: (%s:%d)malloc err %s\n", __FILE__, __LINE__, strerror(errno));
                return NULL;
        }
        ab->empty_list = empty;
        ab->full_list = full;
        return ab;
}

void audio_buf_clear(void *ab)
{
        void *empty = NULL;
        void *full = NULL;
        void *node = NULL;
        struct audio_buf *abuf = ab;
        if (NULL == abuf) {
                printf("err: (%s:%d)malloc err %s\n", __FILE__, __LINE__, strerror(errno));
                return;
        }
        empty = abuf->empty_list;
        if (NULL == empty) {
                printf("err: (%s:%d)malloc err %s\n", __FILE__, __LINE__, strerror(errno));
                return;
        }
        full = abuf->full_list;
        if (NULL == full) {
                printf("err: (%s:%d)malloc err %s\n", __FILE__, __LINE__, strerror(errno));
                return;
        }
        while (NULL != (node = audio_buf_get_node(abuf, AUDIO_BUF_FULL))) {
                audio_buf_put_node(abuf, node, AUDIO_BUF_EMPTY);
        }
        return;
}

void audio_buf_free(void *ab)
{
        struct audio_buf *a = ab;
        if (NULL != a) {
                buf_list_free(a->empty_list);
                buf_list_free(a->full_list);
                free(a);
        } else {
                printf("warn: (%s:%d)blist == NULL\n", __FILE__, __LINE__);
        }
}

void *audio_buf_get_node(void *audio_buf, enum audio_buf_type type)
{
        struct buf_list *empty = NULL;
        struct buf_list *full = NULL;
        struct audio_buf *ab;
        ab = audio_buf;
        if (NULL == ab) {
                printf("err: (%s:%d)ab == NULL\n", __FILE__, __LINE__);
                return NULL;
        }
        empty = ab->empty_list;
        full = ab->full_list;
        if (AUDIO_BUF_EMPTY == type) {
                return buf_list_get_node(empty);
        } else if (AUDIO_BUF_FULL == type) {
                return buf_list_get_node(full);
        } else {
                printf("err: (%s:%d)audio buf type %d\n", __FILE__, __LINE__, type);
                return NULL;
        }
}

void *audio_buf_try_get_node(void *audio_buf, enum audio_buf_type type)
{
        struct buf_list *empty = NULL;
        struct buf_list *full = NULL;
        struct audio_buf *ab;
        ab = audio_buf;
        if (NULL == ab) {
                printf("err: (%s:%d)ab == NULL\n", __FILE__, __LINE__);
                return NULL;
        }
        empty = ab->empty_list;
        full = ab->full_list;
        if (AUDIO_BUF_EMPTY == type) {
                return buf_list_try_get_node(empty);
        } else if (AUDIO_BUF_FULL == type) {
                return buf_list_try_get_node(full);
        } else {
                printf("err: (%s:%d)audio buf type %d\n", __FILE__, __LINE__, type);
                return NULL;
        }
}

void audio_buf_put_node(void *audio_buf, void *node, enum audio_buf_type type)
{
        struct buf_list *empty = NULL;
        struct buf_list *full = NULL;
        struct audio_buf *ab;
        ab = audio_buf;
        if (NULL == ab) {
                printf("err: (%s:%d)ab == NULL\n", __FILE__, __LINE__);
                return;
        }
        empty = ab->empty_list;
        full = ab->full_list;
        if (AUDIO_BUF_EMPTY == type) {
                buf_list_put_node(empty, node);
        } else if (AUDIO_BUF_FULL == type) {
                buf_list_put_node(full, node);
        } else {
                printf("err: (%s:%d)audio buf type %d\n", __FILE__, __LINE__, type);
                return;
        }
}

uint32_t audio_buf_get_num(void *audio_buf, enum audio_buf_type type)
{
        struct buf_list *empty = NULL;
        struct buf_list *full = NULL;
        struct audio_buf *ab;
        ab = audio_buf;
        if (NULL == ab) {
                printf("err: (%s:%d)ab == NULL\n", __FILE__, __LINE__);
                return 0;
        }
        empty = ab->empty_list;
        full = ab->full_list;
        if (AUDIO_BUF_EMPTY == type) {
                return buf_list_get_num(empty);
        } else if (AUDIO_BUF_FULL == type) {
                return buf_list_get_num(full);
        } else {
                printf("err: (%s:%d)audio buf type %d\n", __FILE__, __LINE__, type);
                return 0;
        }
}

void *audio_buf_node_get_info(void *node)
{
        return buf_list_node_get_info(node);
}

void *audio_buf_node_get_data(void *node)
{
        return buf_list_node_get_data(node);
}

int audio_buf_node_index(void *node)
{
        return buf_list_node_index(node);
}

