#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <assert.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <pthread.h>
#include <linux/types.h>
#include "log.h"
#include "impp_fifo.h"
#include "codec_common.h"

#define jpegd_DEV_PAHT	"/dev/jpegdec"
#define TAG				"JPEGD_V2"
#define MAX_BUF_NUM		3

#define PAGE_SIZE 4096
#define alignment_down(a, size) (a & (~(size-1)) )
#define alignment_up(a, size) ((a+size-1) & (~ (size-1)))


#define JZ_JPEGD_IO_MAGIC   'J'
#define JZ_JPEGD_START          _IO(JZ_JPEGD_IO_MAGIC,100)
#define JZ_JPEGD_ALLOC_DMABUF   _IO(JZ_JPEGD_IO_MAGIC,101)
#define JZ_JPEGD_ALLOC_BUFFER   _IO(JZ_JPEGD_IO_MAGIC,102)
#define JZ_JPEGD_FREE_BUFFER    _IO(JZ_JPEGD_IO_MAGIC,103)
#define JZ_JPEGD_FLUSH_CACHE    _IO(JZ_JPEGD_IO_MAGIC,104)
#define JZ_JPEGD_FLUSH_DMACACHE _IO(JZ_JPEGD_IO_MAGIC,105)
#define JZ_JPEGD_GET_DMABUF_PADDR _IO(JZ_JPEGD_IO_MAGIC,106)


typedef struct {
	IHAL_INT32 fd;

	IMPP_BUFFER_TYPE src_buf_type;
	IMPP_BufferInfo_t src_buffer[MAX_BUF_NUM];
	IHAL_INT16 src_buf_num;

	IMPP_BUFFER_TYPE dst_buf_type;
	IHAL_CodecStreamInfo_t dst_buffer[MAX_BUF_NUM];
	IHAL_INT16 dst_buf_num;

	IHAL_INT16 image_width;
	IHAL_INT16 image_height;
	IHAL_INT16 max_width;
	IHAL_INT16 max_height;
	IMPP_PIX_FMT dst_fmt;

	// buffer
    IHAL_UINT32 srcbuf_size;
	struct impp_fifo src_empty_fifo;
	struct impp_fifo src_full_fifo;
	struct impp_fifo dst_empty_fifo;
	struct impp_fifo dst_full_fifo;

	// buffer state
    sem_t src_available;
    sem_t dst_available;

	pthread_t worktid;

	int chn_state;  // 1-->run  0--->stop
	pthread_mutex_t jpegd_mutex;
}jpegd_chn_ctx_t;

struct jpegd_param{
	unsigned int file_size;
	unsigned int src_paddr;
	unsigned int dst_paddr;
	IMPP_PIX_FMT out_fmt;
};

struct jpegd_out_info{
	unsigned int yuv_type;
	unsigned short width;
	unsigned short height;
	unsigned int size;
};

struct jpegd_info {
	struct jpegd_param in_param;
	struct jpegd_out_info out_info;
};

struct jpeg_dmabuf_info{
	unsigned int fd;
	unsigned int size;
	unsigned int paddr;
};

static void* jpegd_chn_ctx_create(void)
{
	int ret = 0;
	jpegd_chn_ctx_t *ctx = calloc(1,sizeof(jpegd_chn_ctx_t));
	if(!ctx){
		IMPP_LOGE(TAG,"init ctx failed");
		return NULL;
	}
	ctx->fd = open(jpegd_DEV_PAHT,O_RDWR);
	if(ctx->fd <= 0){
		free(ctx);
		IMPP_LOGE(TAG,"open %s failed",jpegd_DEV_PAHT);
		return NULL;
	}
	printf("\r\n dev fd = %d \r\n",ctx->fd);
	pthread_mutex_init(&ctx->jpegd_mutex,NULL);
	sem_init(&ctx->src_available,0,0);
	sem_init(&ctx->dst_available,0,0);
	ctx->worktid = 0;
	return ctx;
}

static int jpegd_chn_ctx_destroy(void* chn_ctx)
{
	assert(chn_ctx);
	jpegd_chn_ctx_t *ctx = (jpegd_chn_ctx_t*)chn_ctx;
	pthread_cancel(ctx->worktid);
	pthread_join(ctx->worktid,NULL);
	impp_fifo_deinit(&ctx->src_empty_fifo);
	impp_fifo_deinit(&ctx->src_full_fifo);
	impp_fifo_deinit(&ctx->dst_empty_fifo);
	impp_fifo_deinit(&ctx->dst_full_fifo);
	sem_destroy(&ctx->src_available);
	sem_destroy(&ctx->dst_available);
	close(ctx->fd);
	free(ctx);
	return 0;
}

void* _jpegd_chn_thread(void* arg)
{
	jpegd_chn_ctx_t *ctx = (jpegd_chn_ctx_t*)arg;
	struct jpegd_info info;
	IMPP_BufferInfo_t *srcbuf;
	IMPP_BufferInfo_t *dstbuf;
	int ret = 0;
	for(;;){
		if(ctx->chn_state == 1){
			srcbuf = impp_fifo_dequeue(&ctx->src_full_fifo,IMPP_WAIT_FOREVER);
			dstbuf = impp_fifo_dequeue(&ctx->dst_empty_fifo,IMPP_WAIT_FOREVER);
			memset(&info,0,sizeof(info));
			pthread_mutex_lock(&ctx->jpegd_mutex);
			info.in_param.src_paddr = srcbuf->paddr;
			info.in_param.file_size = srcbuf->byteused;
			info.in_param.dst_paddr = dstbuf->paddr;
			info.in_param.out_fmt = ctx->dst_fmt;
			pthread_mutex_unlock(&ctx->jpegd_mutex);
			ret = ioctl(ctx->fd,JZ_JPEGD_START,&info);
			if(ret){
				IMPP_LOGE(TAG,"jpeg encoder process failed !!!");
				impp_fifo_queue(&ctx->src_empty_fifo,&ctx->src_buffer[srcbuf->index],IMPP_NO_WAIT);
				sem_post(&ctx->src_available);
				impp_fifo_queue(&ctx->dst_empty_fifo,&ctx->dst_buffer[dstbuf->index],IMPP_NO_WAIT);
			} else {
				ctx->dst_buffer[dstbuf->index].mp.len[0] = info.out_info.size;
				ctx->dst_buffer[dstbuf->index].width = info.out_info.width;
				ctx->dst_buffer[dstbuf->index].height = info.out_info.height;
				impp_fifo_queue(&ctx->src_empty_fifo,&ctx->src_buffer[srcbuf->index],IMPP_NO_WAIT);
				impp_fifo_queue(&ctx->dst_full_fifo,&ctx->dst_buffer[dstbuf->index],IMPP_NO_WAIT);
				sem_post(&ctx->src_available);
				sem_post(&ctx->dst_available);
			}
		} else {
			usleep(5 * 1000);
		}
	}
}

static int jpegd_chn_ctx_start(void* chn_ctx)
{
	assert(chn_ctx);
	jpegd_chn_ctx_t *ctx = (jpegd_chn_ctx_t*)chn_ctx;
	pthread_mutex_lock(&ctx->jpegd_mutex);
	if(!ctx->worktid)
		pthread_create(&ctx->worktid,NULL,_jpegd_chn_thread,chn_ctx);
	ctx->chn_state = 1;
	pthread_mutex_unlock(&ctx->jpegd_mutex);
	return 0;
}

static int jpegd_chn_ctx_stop(void* chn_ctx)
{
	assert(chn_ctx);
	jpegd_chn_ctx_t *ctx = (jpegd_chn_ctx_t*)chn_ctx;
	pthread_mutex_lock(&ctx->jpegd_mutex);
	ctx->chn_state = 0;
	pthread_mutex_unlock(&ctx->jpegd_mutex);
	return 0;
}

static _jpegd_check_dst_fmt(IMPP_PIX_FMT dst_fmt)
{
	switch (dst_fmt) {
		case IMPP_PIX_FMT_NV12:
		case IMPP_PIX_FMT_NV21:
		case IMPP_PIX_FMT_YUV444:
		case IMPP_PIX_FMT_YUYV:
		case IMPP_PIX_FMT_RGB_888:
		case IMPP_PIX_FMT_BGRA_8888:
			return 0;
		default:
			IMPP_LOGE(TAG, "dst_fmt type(%d) error!", dst_fmt);
			return -1;
	}
}

static int jpegd_param_set(void* chn_ctx,IHal_CodecParam *param)
{
	assert(chn_ctx);
	jpegd_chn_ctx_t *ctx = (jpegd_chn_ctx_t*)chn_ctx;
	int s_width = param->codecparam.jpegdec_param.src_width;
	int s_height = param->codecparam.jpegdec_param.src_height;

	if (_jpegd_check_dst_fmt(param->codecparam.jpegdec_param.dst_fmt))
		return -1;

	pthread_mutex_lock(&ctx->jpegd_mutex);
	// first set param is the max size
	if(ctx->max_width == 0 && ctx->max_height == 0){
		ctx->max_width = s_width;
		ctx->max_height = s_height;
	} else {
		if(s_width > ctx->max_width || s_height > ctx->max_height){
			IMPP_LOGE(TAG,"chan max size : %dx%d",ctx->max_width,ctx->max_height);
			pthread_mutex_unlock(&ctx->jpegd_mutex);
			return -1;
		}
	}

	ctx->image_width = s_width;
	ctx->image_height = s_height;
    ctx->srcbuf_size = param->codecparam.jpegdec_param.srcbuf_size;
	ctx->dst_fmt	= param->codecparam.jpegdec_param.dst_fmt;

	pthread_mutex_unlock(&ctx->jpegd_mutex);
	return 0;
}

static int jpegd_param_get(void* chn_ctx,IHal_CodecParam *param)
{
	assert(chn_ctx);
	jpegd_chn_ctx_t *ctx = (jpegd_chn_ctx_t*)chn_ctx;

	pthread_mutex_lock(&ctx->jpegd_mutex);
	param->codecparam.jpegdec_param.src_width = ctx->image_width;
	param->codecparam.jpegdec_param.src_height = ctx->image_height;
	param->codecparam.jpegdec_param.dst_fmt = ctx->dst_fmt;
    param->codecparam.jpegdec_param.srcbuf_size = ctx->srcbuf_size;
	pthread_mutex_unlock(&ctx->jpegd_mutex);

	return 0;
}


static int jpegd_src_buffer_create(void* chn_ctx,IMPP_BUFFER_TYPE type,int num)
{
	int ret = 0;
	int i = 0;
	struct jpeg_dmabuf_info bufinfo;
	assert(chn_ctx);
	jpegd_chn_ctx_t *ctx = (jpegd_chn_ctx_t*)chn_ctx;

	unsigned int bufsize = ctx->srcbuf_size;

	//printf("\r\n buffer size = %d \r\n",bufsize);
	if(num > MAX_BUF_NUM){
		IMPP_LOGW(TAG,"max buffer num = %d ",MAX_BUF_NUM);
		num = MAX_BUF_NUM;
	}
	ret = impp_fifo_init(&ctx->src_empty_fifo,num);
	if(ret){
		IMPP_LOGE(TAG,"init src empty fifo err");
		return -1;
	}
	ret = impp_fifo_init(&ctx->src_full_fifo,num);
	if(ret){
		IMPP_LOGE(TAG,"init src full fifo err");
		return -1;
	}
	ctx->src_buf_type = type;
	if(type == IMPP_INTERNAL_BUFFER) {
		for(i = 0; i < num; i++){
			bufinfo.size = bufsize;
			bufinfo.paddr = 0;
			bufinfo.fd = 0;
			ret = ioctl(ctx->fd,JZ_JPEGD_ALLOC_DMABUF,&bufinfo);
			if(ret){
				IMPP_LOGE(TAG,"alloc dmabuf err");
				return -1;
			}
			ctx->src_buffer[i].size = bufsize;
			ctx->src_buffer[i].fd   = bufinfo.fd;
			ctx->src_buffer[i].paddr = bufinfo.paddr;
			ctx->src_buffer[i].index = i;
			ctx->src_buffer[i].vaddr = (unsigned int)mmap(0,bufsize,PROT_READ | PROT_WRITE,MAP_SHARED,bufinfo.fd,0);
			impp_fifo_queue(&ctx->src_empty_fifo,&ctx->src_buffer[i],IMPP_NO_WAIT);
			sem_post(&ctx->src_available);
		}
	} else if(type == IMPP_EXT_DMABUFFER) {

	} else {
		IMPP_LOGE(TAG,"not support user buffer");
		return -1;
	}
	ctx->src_buf_num = num;
	return num;
}

static int jpegd_src_buffer_free(void* chn_ctx)
{
	assert(chn_ctx);
	jpegd_chn_ctx_t *ctx = (jpegd_chn_ctx_t*)chn_ctx;

	if(ctx->src_buf_type == IMPP_INTERNAL_BUFFER){
		for(int i = 0; i < ctx->src_buf_num; i++){
			munmap(ctx->src_buffer[i].vaddr,ctx->src_buffer[i].size);
			close(ctx->src_buffer[i].fd);
		}
	}

	return 0;
}

static int jpegd_dst_buffer_create(void* chn_ctx,IMPP_BUFFER_TYPE type,int num)
{
	int ret = 0;
	int i = 0;
	struct jpeg_dmabuf_info bufinfo;
	unsigned int bufsize = 0;

	assert(chn_ctx);
	jpegd_chn_ctx_t *ctx = (jpegd_chn_ctx_t*)chn_ctx;

	switch(ctx->dst_fmt){
		case IMPP_PIX_FMT_NV12:
		case IMPP_PIX_FMT_NV21:
			bufsize = alignment_up(ctx->image_width * ctx->image_height * 3 / 2,PAGE_SIZE);
			break;
		case IMPP_PIX_FMT_YUYV:
			bufsize = alignment_up(ctx->image_width * ctx->image_height * 2,PAGE_SIZE);
			break;
		default:
			bufsize = alignment_up(ctx->image_width * ctx->image_height * 4,PAGE_SIZE);
			break;
	}
	if(num > MAX_BUF_NUM){
		IMPP_LOGW(TAG,"max buffer num = %d ",MAX_BUF_NUM);
		num = MAX_BUF_NUM;
	}
	ret = impp_fifo_init(&ctx->dst_empty_fifo,num);
	if(ret){
		IMPP_LOGE(TAG,"init dst empty fifo err");
		return -1;
	}
	ret = impp_fifo_init(&ctx->dst_full_fifo,num);
	if(ret){
		IMPP_LOGE(TAG,"init dst full fifo err");
		return -1;
	}
	ctx->dst_buf_type = type;
	if(type == IMPP_INTERNAL_BUFFER) {
		for(i = 0; i < num; i++){
			bufinfo.size = bufsize;
			ret = ioctl(ctx->fd,JZ_JPEGD_ALLOC_DMABUF,&bufinfo);
			if(ret){
				IMPP_LOGE(TAG,"alloc dmabuf err");
				return -1;
			}
			ctx->dst_buffer[i].size = bufsize;
			ctx->dst_buffer[i].fd   = bufinfo.fd;
			ctx->dst_buffer[i].paddr = bufinfo.paddr;
			ctx->dst_buffer[i].vaddr = (unsigned int)mmap(0,bufsize, PROT_READ | PROT_WRITE,MAP_SHARED,bufinfo.fd,0);
			ctx->dst_buffer[i].index = i;
			impp_fifo_queue(&ctx->dst_empty_fifo,&ctx->dst_buffer[i],IMPP_NO_WAIT);
		}
	} else if(type == IMPP_EXT_DMABUFFER) {
	} else if(type == IMPP_EXT_USERBUFFER) {

	} else {
		IMPP_LOGE(TAG,"not support user buffer");
		return -1;
	}

	ctx->dst_buf_num = num;
	return num;
}

static int jpegd_dst_buffer_free(void* chn_ctx)
{
	assert(chn_ctx);
	jpegd_chn_ctx_t *ctx = (jpegd_chn_ctx_t*)chn_ctx;
	if(ctx->dst_buf_type == IMPP_INTERNAL_BUFFER){
		for(int i = 0; i < ctx->dst_buf_num; i++){
			munmap(ctx->dst_buffer[i].vaddr,ctx->dst_buffer[i].size);
			close(ctx->dst_buffer[i].fd);
		}
	}

	return 0;
}

static int jpegd_get_src_buffer(void* chn_ctx,IMPP_BufferInfo_t *buffer,int index)
{
	assert(chn_ctx);
	jpegd_chn_ctx_t *ctx = (jpegd_chn_ctx_t*)chn_ctx;

	pthread_mutex_lock(&ctx->jpegd_mutex);
	if(index > (ctx->src_buf_num-1))
		return -1;
	if(ctx->src_buf_type == IMPP_INTERNAL_BUFFER){
		buffer->fd = ctx->src_buffer[index].fd;
		buffer->size = ctx->src_buffer[index].size;
		buffer->paddr = ctx->src_buffer[index].paddr;
		buffer->vaddr = ctx->src_buffer[index].vaddr;
	} else {
		IMPP_LOGE(TAG,"src buffer type is not internal buffer,can not get share mem");
		pthread_mutex_unlock(&ctx->jpegd_mutex);
		return -1;
	}
	pthread_mutex_unlock(&ctx->jpegd_mutex);

	return 0;
}

static int jpegd_set_src_buffer(void* chn_ctx,IMPP_BufferInfo_t *buffer,int index)
{
	int ret = 0;
	struct jpeg_dmabuf_info bufinfo;
	assert(chn_ctx);
	jpegd_chn_ctx_t *ctx = (jpegd_chn_ctx_t*)chn_ctx;

	pthread_mutex_lock(&ctx->jpegd_mutex);
	if(index > (ctx->src_buf_num-1))
		return -1;
	if(ctx->src_buf_type == IMPP_EXT_DMABUFFER){
		ctx->src_buffer[index].fd = buffer->fd;
		ctx->src_buffer[index].size = buffer->size;
		// get paddr
		bufinfo.fd = buffer->fd;
		bufinfo.size = buffer->size;
		bufinfo.paddr = 0;
		ret = ioctl(ctx->fd,JZ_JPEGD_GET_DMABUF_PADDR,&bufinfo);
		if(ret){
			IMPP_LOGE(TAG,"get share dmabuf paddr failed");
			pthread_mutex_unlock(&ctx->jpegd_mutex);
			return -1;
		}
		ctx->src_buffer[index].paddr = bufinfo.paddr;
		impp_fifo_queue(&ctx->src_empty_fifo,&ctx->src_buffer[index],IMPP_NO_WAIT);
		sem_post(&ctx->src_available);
	} else {
		IMPP_LOGE(TAG,"src buffer type is not internal buffer,can not get share mem");
		pthread_mutex_unlock(&ctx->jpegd_mutex);
		return -1;
	}
	pthread_mutex_unlock(&ctx->jpegd_mutex);

	return 0;
}

static int jpegd_get_dst_buffer(void* chn_ctx,IMPP_BufferInfo_t *buffer,int index)
{
	assert(chn_ctx);
	jpegd_chn_ctx_t *ctx = (jpegd_chn_ctx_t*)chn_ctx;

	pthread_mutex_lock(&ctx->jpegd_mutex);
	if(index > (ctx->dst_buf_num - 1))
		return -1;
	if(ctx->dst_buf_type == IMPP_INTERNAL_BUFFER){
		buffer->fd    = ctx->dst_buffer[index].fd;
		buffer->size  = ctx->dst_buffer[index].size;
		buffer->paddr = ctx->dst_buffer[index].paddr;
		buffer->vaddr = ctx->dst_buffer[index].vaddr;
	} else {
		IMPP_LOGE(TAG,"src buffer type is not internal buffer,can not get share mem");
		pthread_mutex_unlock(&ctx->jpegd_mutex);
		return -1;
	}
	pthread_mutex_unlock(&ctx->jpegd_mutex);

	return 0;
}

static int jpegd_set_dst_buffer(void* chn_ctx,IMPP_BufferInfo_t *buffer,int index)
{
	int ret = 0;
	struct jpeg_dmabuf_info bufinfo;
	assert(chn_ctx);
	jpegd_chn_ctx_t *ctx = (jpegd_chn_ctx_t*)chn_ctx;

	pthread_mutex_lock(&ctx->jpegd_mutex);
	if(index > (ctx->src_buf_num-1))
		return -1;
	if(ctx->dst_buf_type == IMPP_EXT_DMABUFFER){
		ctx->dst_buffer[index].fd = buffer->fd;
		ctx->dst_buffer[index].size = buffer->size;
		// get paddr
		bufinfo.fd = buffer->fd;
		bufinfo.size = buffer->size;
		bufinfo.paddr = 0;
		ret = ioctl(ctx->fd,JZ_JPEGD_GET_DMABUF_PADDR,&bufinfo);
		if(ret){
			IMPP_LOGE(TAG,"get share dmabuf paddr failed");
			pthread_mutex_unlock(&ctx->jpegd_mutex);
			return -1;
		}
		ctx->dst_buffer[index].paddr = bufinfo.paddr;
		impp_fifo_queue(&ctx->dst_empty_fifo,&ctx->dst_buffer[index],IMPP_NO_WAIT);
    }else if(ctx->dst_buf_type == IMPP_EXT_USERBUFFER){
		ctx->dst_buffer[index].size = buffer->size;
		ctx->dst_buffer[index].vaddr = buffer->vaddr;
		ctx->dst_buffer[index].paddr = buffer->paddr;
		impp_fifo_queue(&ctx->dst_empty_fifo,&ctx->dst_buffer[index],IMPP_NO_WAIT);
	} else {
		IMPP_LOGE(TAG,"dst buffer type is not internal buffer,can not get share mem");
		pthread_mutex_unlock(&ctx->jpegd_mutex);
		return -1;
	}
	pthread_mutex_unlock(&ctx->jpegd_mutex);

	return 0;
}

static int jpegd_poll_src_buffer_avaliable(void* chn_ctx)
{
	assert(chn_ctx);
	jpegd_chn_ctx_t *ctx = (jpegd_chn_ctx_t*)chn_ctx;
	return sem_wait(&ctx->src_available);
}

static int jpegd_poll_dst_buffer_avaliable(void* chn_ctx)
{
	assert(chn_ctx);
	jpegd_chn_ctx_t *ctx = (jpegd_chn_ctx_t*)chn_ctx;
	return sem_wait(&ctx->dst_available);
}

static int jpegd_queue_src_buffer(void* chn_ctx,IMPP_BufferInfo_t *buffer)
{
	assert(chn_ctx);
	jpegd_chn_ctx_t *ctx = (jpegd_chn_ctx_t*)chn_ctx;
	int index = buffer->index;
	if(index > (ctx->src_buf_num - 1)){
		IMPP_LOGE(TAG,"index is not avaliable");
		return -1;
	}
	if(buffer->vaddr == ctx->src_buffer[index].vaddr
			&& buffer->paddr == ctx->src_buffer[index].paddr){
		ctx->src_buffer[index].byteused = buffer->byteused;
		impp_fifo_queue(&ctx->src_full_fifo,&ctx->src_buffer[index],IMPP_WAIT_FOREVER);
	} else {
		IMPP_LOGE(TAG,"the buffer is not for  this channel");
		return -1;
	}
	return 0;
}

static int jpegd_dequeue_src_buffer(void* chn_ctx,IMPP_BufferInfo_t *buffer)
{
	assert(chn_ctx);
	jpegd_chn_ctx_t *ctx = (jpegd_chn_ctx_t*)chn_ctx;
	IMPP_BufferInfo_t *buf = NULL;
	buf = impp_fifo_dequeue(&ctx->src_empty_fifo,IMPP_WAIT_FOREVER);
	if(!buf){
		IMPP_LOGE(TAG,"get src empty buffer failed");
		return -1;
	}
	buffer->fd = buf->fd;
	buffer->vaddr = buf->vaddr;
	buffer->paddr = buf->paddr;
	buffer->index = buf->index;

	return 0;
}

static int jpegd_queue_dst_buffer(void* chn_ctx,IHAL_CodecStreamInfo_t *buffer)
{
	assert(chn_ctx);
	jpegd_chn_ctx_t *ctx = (jpegd_chn_ctx_t*)chn_ctx;
	int index = buffer->index;
	if(index > (ctx->dst_buf_num - 1)){
		IMPP_LOGE(TAG,"index is not avaliable");
		return -1;
	}
	if(buffer->vaddr == ctx->dst_buffer[index].vaddr
			&& buffer->paddr == ctx->dst_buffer[index].paddr){
		impp_fifo_queue(&ctx->dst_empty_fifo,&ctx->dst_buffer[index],IMPP_NO_WAIT);
	} else {
		IMPP_LOGE(TAG,"the buffer is not for  this channel");
		return -1;
	}
	return 0;
}

static int jpegd_dequeue_dst_buffer(void* chn_ctx,IHAL_CodecStreamInfo_t *buffer)
{
	assert(chn_ctx);
	jpegd_chn_ctx_t *ctx = (jpegd_chn_ctx_t*)chn_ctx;
	IHAL_CodecStreamInfo_t *buf = NULL;
	buf = impp_fifo_dequeue(&ctx->dst_full_fifo,IMPP_WAIT_FOREVER);
	if(!buf){
		IMPP_LOGE(TAG,"get src empty buffer failed");
		return -1;
	}
	buffer->fd = buf->fd;
	buffer->vaddr = buf->vaddr;
	buffer->paddr = buf->paddr;
	buffer->index = buf->index;
	buffer->size = buf->mp.len[0];
	buffer->width = buf->width;
	buffer->height = buf->height;
	return 0;
}


ImppCodec_t jpegdec_v2 = {
	.name = "jpeg_dec_v2",
	.version = 2023042701,
	.codec_init = NULL,
	.codec_deinit = NULL,
	.codec_chn_ctx_create = jpegd_chn_ctx_create,
	.codec_chn_ctx_destroy = jpegd_chn_ctx_destroy,

	.codec_chn_start = jpegd_chn_ctx_start,
	.codec_chn_stop  = jpegd_chn_ctx_stop,
	.codec_param_set = jpegd_param_set,
	.codec_param_get = jpegd_param_get,

	.src_buffer_create = jpegd_src_buffer_create,
	.src_buffer_free   = jpegd_src_buffer_free,
	.dst_buffer_create = jpegd_dst_buffer_create,
	.dst_buffer_free   = jpegd_dst_buffer_free,

	.get_src_buffer = jpegd_get_src_buffer,
	.set_src_buffer = jpegd_set_src_buffer,
	.get_dst_buffer = jpegd_get_dst_buffer,
	.set_dst_buffer = jpegd_set_dst_buffer,

	.poll_src_buffer_avaliable = jpegd_poll_src_buffer_avaliable,
	.poll_dst_buffer_avaliable = jpegd_poll_dst_buffer_avaliable,

	.queue_src_buffer = jpegd_queue_src_buffer,
	.dequeue_src_buffer = jpegd_dequeue_src_buffer,
	.queue_dst_buffer = jpegd_queue_dst_buffer,
	.dequeue_dst_buffer = jpegd_dequeue_dst_buffer,

};



