#ifndef __AUDIO_BUF_H__
#define __AUDIO_BUF_H__
#include <stdint.h>
#define container_of(ptr, type, member) ({\
                const typeof(((type *) 0)->member) *__mptr = (ptr);\
                (type *) ((char *) __mptr - offsetof(type, member));})


enum audio_buf_type {
        AUDIO_BUF_EMPTY,
        AUDIO_BUF_FULL,
};

void *audio_buf_alloc(uint32_t num, uint32_t data_size, uint32_t info_size);
void audio_buf_free(void *ab);
void audio_buf_clear(void *ab);
void *audio_buf_get_node(void *audio_buf, enum audio_buf_type type);
void *audio_buf_try_get_node(void *audio_buf, enum audio_buf_type type);
void audio_buf_put_node(void *audio_buf, void *node, enum audio_buf_type type);
uint32_t audio_buf_get_num(void *audio_buf, enum audio_buf_type type);
void *audio_buf_node_get_info(void *node);
void *audio_buf_node_get_data(void *node);
int audio_buf_node_index(void *node);

#endif /* __AUDIO_BUF_H__ */
