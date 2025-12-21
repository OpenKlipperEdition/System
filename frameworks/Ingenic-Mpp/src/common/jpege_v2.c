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

#define JPEGE_DEV_PAHT	"/dev/jpegenc"
#define TAG				"JPEGE_V2"
#define MAX_BUF_NUM		3
#define JPG_FILE_MAX_SIZE  (512*1024)

#define PAGE_SIZE 4096
#define alignment_down(a, size) (a & (~(size-1)) )
#define alignment_up(a, size) ((a+size-1) & (~ (size-1)))


#define JZ_JPEGE_IO_MAGIC   'J'
#define JZ_JPEGE_START          _IO(JZ_JPEGE_IO_MAGIC,100)
#define JZ_JPEGE_ALLOC_DMABUF   _IO(JZ_JPEGE_IO_MAGIC,101)
#define JZ_JPEGE_ALLOC_BUFFER   _IO(JZ_JPEGE_IO_MAGIC,102)
#define JZ_JPEGE_FREE_BUFFER    _IO(JZ_JPEGE_IO_MAGIC,103)
#define JZ_JPEGE_FLUSH_CACHE    _IO(JZ_JPEGE_IO_MAGIC,104)
#define JZ_JPEGE_FLUSH_DMACACHE _IO(JZ_JPEGE_IO_MAGIC,105)
#define JZ_JPEGE_GET_DMABUF_PADDR _IO(JZ_JPEGE_IO_MAGIC,106)


typedef struct {
	IHAL_INT32 fd;

	IMPP_BUFFER_TYPE src_buf_type;
	IMPP_BufferInfo_t src_buffer[MAX_BUF_NUM];
	IHAL_INT16 src_buf_num;

	IMPP_BUFFER_TYPE dst_buf_type;
	IMPP_BufferInfo_t dst_buffer[MAX_BUF_NUM];
	IHAL_INT16 dst_buf_num;

	IHAL_INT16 image_width;
	IHAL_INT16 image_height;
	IHAL_INT16 max_width;
	IHAL_INT16 max_height;
	IMPP_PIX_FMT src_fmt;
	int qa;	// jpeg  encoder quality

	// buffer
	struct impp_fifo src_empty_fifo;
	struct impp_fifo src_full_fifo;
	struct impp_fifo dst_empty_fifo;
	struct impp_fifo dst_full_fifo;

	// buffer state
    sem_t src_available;
    sem_t dst_available;

	pthread_t worktid;

	int chn_state;  // 1-->run  0--->stop
	pthread_mutex_t jpege_mutex;
}jpege_chn_ctx_t;

struct jpege_in_info {
	unsigned int src_paddr;
	unsigned int dst_paddr;
	unsigned int dst_bufsize;
	unsigned short frame_width;
	unsigned short frame_height;
	short qa;
	IMPP_PIX_FMT pix_fmt;
	char subsamp;
	unsigned short rstm_request;
};

struct jpege_out_info{
	unsigned int imagesize;
};

struct jpege_info {
	struct jpege_in_info in;
	struct jpege_out_info out;
};

struct jpeg_dmabuf_info{
	unsigned int fd;
	unsigned int size;
	unsigned int paddr;
};

static void* jpege_chn_ctx_create(void)
{
	int ret = 0;
	jpege_chn_ctx_t *ctx = calloc(1,sizeof(jpege_chn_ctx_t));
	if(!ctx){
		IMPP_LOGE(TAG,"init ctx failed");
		return NULL;
	}
	ctx->fd = open(JPEGE_DEV_PAHT,O_RDWR);
	if(ctx->fd <= 0){
		free(ctx);
		IMPP_LOGE(TAG,"open %s failed",JPEGE_DEV_PAHT);
		return NULL;
	}
	printf("\r\n dev fd = %d \r\n",ctx->fd);
	pthread_mutex_init(&ctx->jpege_mutex,NULL);
	sem_init(&ctx->src_available,0,0);
	sem_init(&ctx->dst_available,0,0);
	ctx->worktid = 0;
	return ctx;
}

static int jpege_chn_ctx_destroy(void* chn_ctx)
{
	assert(chn_ctx);
	jpege_chn_ctx_t *ctx = (jpege_chn_ctx_t*)chn_ctx;
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

void* _jpege_chn_thread(void* arg)
{
	jpege_chn_ctx_t *ctx = (jpege_chn_ctx_t*)arg;
	struct jpege_info info;
	IMPP_BufferInfo_t *srcbuf;
	IMPP_BufferInfo_t *dstbuf;
	int ret = 0;
	for(;;){
		if(ctx->chn_state == 1){
			srcbuf = impp_fifo_dequeue(&ctx->src_full_fifo,IMPP_WAIT_FOREVER);
			dstbuf = impp_fifo_dequeue(&ctx->dst_empty_fifo,IMPP_WAIT_FOREVER);
			memset(&info,0,sizeof(info));
			info.in.src_paddr = srcbuf->paddr;
			info.in.dst_paddr = dstbuf->paddr;
			info.in.dst_bufsize = dstbuf->size / 4096;	// 4K unit
			info.in.subsamp = 1;
			info.in.rstm_request = 0;
			pthread_mutex_lock(&ctx->jpege_mutex);
			info.in.frame_width = ctx->image_width;
			info.in.frame_height = ctx->image_height;
			info.in.qa = ctx->qa;
			info.in.pix_fmt = ctx->src_fmt;
			pthread_mutex_unlock(&ctx->jpege_mutex);
			ret = ioctl(ctx->fd,JZ_JPEGE_START,&info);
			if(ret){
				IMPP_LOGE(TAG,"jpeg encoder process failed !!!");
				impp_fifo_queue(&ctx->src_empty_fifo,&ctx->src_buffer[srcbuf->index],IMPP_NO_WAIT);
				sem_post(&ctx->src_available);
				impp_fifo_queue(&ctx->dst_empty_fifo,&ctx->dst_buffer[dstbuf->index],IMPP_NO_WAIT);
			} else {
				ctx->dst_buffer[dstbuf->index].byteused = info.out.imagesize;
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

static int jpege_chn_ctx_start(void* chn_ctx)
{
	assert(chn_ctx);
	jpege_chn_ctx_t *ctx = (jpege_chn_ctx_t*)chn_ctx;
	pthread_mutex_lock(&ctx->jpege_mutex);
	if(!ctx->worktid)
		pthread_create(&ctx->worktid,NULL,_jpege_chn_thread,chn_ctx);
	ctx->chn_state = 1;
	pthread_mutex_unlock(&ctx->jpege_mutex);
	return 0;
}

static int jpege_chn_ctx_stop(void* chn_ctx)
{
	assert(chn_ctx);
	jpege_chn_ctx_t *ctx = (jpege_chn_ctx_t*)chn_ctx;
	pthread_mutex_lock(&ctx->jpege_mutex);
	ctx->chn_state = 0;
	pthread_mutex_unlock(&ctx->jpege_mutex);
	return 0;
}

static int _jpege_check_src_fmt(IMPP_PIX_FMT src_fmt)
{
	switch (src_fmt) {
		case IMPP_PIX_FMT_NV12:
		case IMPP_PIX_FMT_NV21:
		case IMPP_PIX_FMT_YUYV:
		case IMPP_PIX_FMT_YUV444:
		case IMPP_PIX_FMT_RGB_888:
		case IMPP_PIX_FMT_RGBA_8888:
		case IMPP_PIX_FMT_BGRA_8888:
		case IMPP_PIX_FMT_GREY:
			return 0;
		default:
			IMPP_LOGE(TAG, "src_fmt type(%d) error!", src_fmt);
			return -1;
	}
}

static int jpege_param_set(void* chn_ctx,IHal_CodecParam *param)
{
	assert(chn_ctx);
	assert(param);
	jpege_chn_ctx_t *ctx = (jpege_chn_ctx_t*)chn_ctx;
	int s_width = param->codecparam.jpegenc_param.src_width;
	int s_height = param->codecparam.jpegenc_param.src_height;

	if (_jpege_check_src_fmt(param->codecparam.jpegenc_param.src_fmt))
		return -1;

	pthread_mutex_lock(&ctx->jpege_mutex);
	// first set param is the max size
	if(ctx->max_width == 0 && ctx->max_height == 0){
		ctx->max_width = s_width;
		ctx->max_height = s_height;
	} else {
		if(s_width > ctx->max_width || s_height > ctx->max_height){
			IMPP_LOGE(TAG,"chan max size : %dx%d",ctx->max_width,ctx->max_height);
			pthread_mutex_unlock(&ctx->jpege_mutex);
			return -1;
		}
	}

	ctx->image_width = s_width;
	ctx->image_height = s_height;
	ctx->src_fmt	= param->codecparam.jpegenc_param.src_fmt;
	ctx->qa		    = param->codecparam.jpegenc_param.quality;

	pthread_mutex_unlock(&ctx->jpege_mutex);
	return 0;
}

static int jpege_param_get(void* chn_ctx,IHal_CodecParam *param)
{
	assert(chn_ctx);
	assert(param);
	jpege_chn_ctx_t *ctx = (jpege_chn_ctx_t*)chn_ctx;

	pthread_mutex_lock(&ctx->jpege_mutex);
	param->codecparam.jpegenc_param.src_width = ctx->image_width;
	param->codecparam.jpegenc_param.src_height = ctx->image_height;
	param->codecparam.jpegenc_param.quality = ctx->qa;
	param->codecparam.jpegenc_param.src_fmt = ctx->src_fmt;
	pthread_mutex_unlock(&ctx->jpege_mutex);

	return 0;
}


static int jpege_src_buffer_create(void* chn_ctx,IMPP_BUFFER_TYPE type,int num)
{
	int ret = 0;
	int i = 0;
	struct jpeg_dmabuf_info bufinfo;
	unsigned int bufsize = 0;

	assert(chn_ctx);
	jpege_chn_ctx_t *ctx = (jpege_chn_ctx_t*)chn_ctx;

	switch(ctx->src_fmt){
		case IMPP_PIX_FMT_GREY:
			bufsize = alignment_up(ctx->image_width * ctx->image_height, PAGE_SIZE);
			break;
		case IMPP_PIX_FMT_NV12:
		case IMPP_PIX_FMT_NV21:
			bufsize = alignment_up(ctx->image_width * ctx->image_height * 3 / 2,PAGE_SIZE);
			break;
		case IMPP_PIX_FMT_YUYV:
			bufsize = alignment_up(ctx->image_width * ctx->image_height * 2,PAGE_SIZE);
			break;
		case IMPP_PIX_FMT_RGB_888:
		case IMPP_PIX_FMT_YUV444:
			bufsize = alignment_up(ctx->image_width * ctx->image_height * 3,PAGE_SIZE);
			break;
		default:
			bufsize = alignment_up(ctx->image_width * ctx->image_height * 4,PAGE_SIZE);
			break;
	}
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
			ret = ioctl(ctx->fd,JZ_JPEGE_ALLOC_DMABUF,&bufinfo);
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
	} else if(type == IMPP_EXT_USERBUFFER) {
	} else {
		IMPP_LOGE(TAG,"not support user buffer");
		return -1;
	}
	ctx->src_buf_num = num;
	return num;
}

static int jpege_src_buffer_free(void* chn_ctx)
{
	assert(chn_ctx);
	jpege_chn_ctx_t *ctx = (jpege_chn_ctx_t*)chn_ctx;

	if(ctx->src_buf_type == IMPP_INTERNAL_BUFFER){
		for(int i = 0; i < ctx->src_buf_num; i++){
			munmap(ctx->src_buffer[i].vaddr,ctx->src_buffer[i].size);
			close(ctx->src_buffer[i].fd);
		}
	}

	return 0;
}

static int jpege_dst_buffer_create(void* chn_ctx,IMPP_BUFFER_TYPE type,int num)
{
	int ret = 0;
	int i = 0;
	struct jpeg_dmabuf_info bufinfo;
	unsigned int bufsize = JPG_FILE_MAX_SIZE;

	assert(chn_ctx);
	jpege_chn_ctx_t *ctx = (jpege_chn_ctx_t*)chn_ctx;

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
			ret = ioctl(ctx->fd,JZ_JPEGE_ALLOC_DMABUF,&bufinfo);
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

	} else {
		IMPP_LOGE(TAG,"not support user buffer");
		return -1;
	}

	ctx->dst_buf_num = num;
	return num;
}

static int jpege_dst_buffer_free(void* chn_ctx)
{
	assert(chn_ctx);
	jpege_chn_ctx_t *ctx = (jpege_chn_ctx_t*)chn_ctx;
	if(ctx->dst_buf_type == IMPP_INTERNAL_BUFFER){
		for(int i = 0; i < ctx->dst_buf_num; i++){
			munmap(ctx->dst_buffer[i].vaddr,ctx->dst_buffer[i].size);
			close(ctx->dst_buffer[i].fd);
		}
	}

	return 0;
}

static int jpege_get_src_buffer(void* chn_ctx,IMPP_BufferInfo_t *buffer,int index)
{
	assert(chn_ctx);
	jpege_chn_ctx_t *ctx = (jpege_chn_ctx_t*)chn_ctx;
	IMPP_BufferInfo_t *qbuf = NULL;

	pthread_mutex_lock(&ctx->jpege_mutex);
	if(index > (ctx->src_buf_num-1))
		return -1;
	if(ctx->src_buf_type == IMPP_INTERNAL_BUFFER){
		sem_wait(&ctx->src_available);

		qbuf = impp_fifo_dequeue(&ctx->src_empty_fifo, IMPP_WAIT_FOREVER);
		if (!qbuf) {
			IMPP_LOGE(TAG, "the buf[%d] has been get out \n", index);
			return -IHAL_RFAILED;
		}

		buffer->fd    = qbuf->fd;
		buffer->size  = qbuf->size;
		buffer->paddr = qbuf->paddr;
		buffer->vaddr = qbuf->vaddr;
	} else {
		IMPP_LOGE(TAG,"src buffer type is not internal buffer,can not get share mem");
		pthread_mutex_unlock(&ctx->jpege_mutex);
		return -1;
	}
	pthread_mutex_unlock(&ctx->jpege_mutex);

	return 0;
}

static int jpege_set_src_buffer(void* chn_ctx,IMPP_BufferInfo_t *buffer,int index)
{
	int ret = 0;
	struct jpeg_dmabuf_info bufinfo;
	assert(chn_ctx);
	jpege_chn_ctx_t *ctx = (jpege_chn_ctx_t*)chn_ctx;

	pthread_mutex_lock(&ctx->jpege_mutex);
	if(index > (ctx->src_buf_num-1))
		return -1;
	if(ctx->src_buf_type == IMPP_EXT_DMABUFFER){
		ctx->src_buffer[index].fd = buffer->fd;
		ctx->src_buffer[index].size = buffer->size;
		ctx->src_buffer[index].vaddr = buffer->vaddr;
		// get paddr
		bufinfo.fd = buffer->fd;
		bufinfo.size = buffer->size;
		bufinfo.paddr = 0;
		ret = ioctl(ctx->fd,JZ_JPEGE_GET_DMABUF_PADDR,&bufinfo);
		if(ret){
			IMPP_LOGE(TAG,"get share dmabuf paddr failed");
			pthread_mutex_unlock(&ctx->jpege_mutex);
			return -1;
		}
		ctx->src_buffer[index].paddr = bufinfo.paddr;
    }else if(ctx->src_buf_type == IMPP_EXT_USERBUFFER){
		ctx->src_buffer[index].size = buffer->size;
		ctx->src_buffer[index].vaddr = buffer->vaddr;
		ctx->src_buffer[index].paddr = buffer->paddr;
	} else {
		IMPP_LOGE(TAG,"src buffer type is not internal buffer,can not get share mem");
		pthread_mutex_unlock(&ctx->jpege_mutex);
		return -1;
	}
	pthread_mutex_unlock(&ctx->jpege_mutex);

	return 0;
}

static int jpege_get_dst_buffer(void* chn_ctx,IMPP_BufferInfo_t *buffer,int index)
{
	assert(chn_ctx);
	jpege_chn_ctx_t *ctx = (jpege_chn_ctx_t*)chn_ctx;

	pthread_mutex_lock(&ctx->jpege_mutex);
	if(index > (ctx->dst_buf_num - 1))
		return -1;
	if(ctx->dst_buf_type == IMPP_INTERNAL_BUFFER){
		buffer->fd    = ctx->dst_buffer[index].fd;
		buffer->size  = ctx->dst_buffer[index].size;
		buffer->paddr = ctx->dst_buffer[index].paddr;
		buffer->vaddr = ctx->dst_buffer[index].vaddr;
	} else {
		IMPP_LOGE(TAG,"src buffer type is not internal buffer,can not get share mem");
		pthread_mutex_unlock(&ctx->jpege_mutex);
		return -1;
	}
	pthread_mutex_unlock(&ctx->jpege_mutex);

	return 0;
}

static int jpege_set_dst_buffer(void* chn_ctx,IMPP_BufferInfo_t *buffer,int index)
{
	int ret = 0;
	struct jpeg_dmabuf_info bufinfo;
	assert(chn_ctx);
	jpege_chn_ctx_t *ctx = (jpege_chn_ctx_t*)chn_ctx;

	pthread_mutex_lock(&ctx->jpege_mutex);
	if(index > (ctx->src_buf_num-1))
		return -1;
	if(ctx->dst_buf_type == IMPP_EXT_DMABUFFER){
		ctx->dst_buffer[index].fd = buffer->fd;
		ctx->dst_buffer[index].size = buffer->size;
		// get paddr
		bufinfo.fd = buffer->fd;
		bufinfo.size = buffer->size;
		bufinfo.paddr = 0;
		ret = ioctl(ctx->fd,JZ_JPEGE_GET_DMABUF_PADDR,&bufinfo);
		if(ret){
			IMPP_LOGE(TAG,"get share dmabuf paddr failed");
			pthread_mutex_unlock(&ctx->jpege_mutex);
			return -1;
		}
		ctx->dst_buffer[index].paddr = bufinfo.paddr;
		impp_fifo_queue(&ctx->dst_empty_fifo,&ctx->dst_buffer[index],IMPP_NO_WAIT);
	} else {
		IMPP_LOGE(TAG,"dst buffer type is not internal buffer,can not get share mem");
		pthread_mutex_unlock(&ctx->jpege_mutex);
		return -1;
	}
	pthread_mutex_unlock(&ctx->jpege_mutex);

	return 0;
}

static int jpege_poll_src_buffer_avaliable(void* chn_ctx, int wait_type)
{
	assert(chn_ctx);
	jpege_chn_ctx_t *ctx = (jpege_chn_ctx_t*)chn_ctx;
	return sem_wait(&ctx->src_available);
}

static int jpege_poll_dst_buffer_avaliable(void* chn_ctx, int wait_type)
{
	assert(chn_ctx);
	jpege_chn_ctx_t *ctx = (jpege_chn_ctx_t*)chn_ctx;
	return sem_wait(&ctx->dst_available);
}

static int jpege_queue_src_buffer(void* chn_ctx,IMPP_BufferInfo_t *buffer)
{
	assert(chn_ctx);
	jpege_chn_ctx_t *ctx = (jpege_chn_ctx_t*)chn_ctx;
	int ret = 0;
	int index = buffer->index;
	if(index > (ctx->src_buf_num - 1)){
		IMPP_LOGE(TAG,"index is not avaliable");
		return -1;
	}

	if(ctx->src_buf_type == IMPP_EXT_USERBUFFER
			&& buffer->index == ctx->src_buffer[index].index) {
		impp_fifo_queue(&ctx->src_full_fifo,&ctx->src_buffer[index],IMPP_WAIT_FOREVER);
    } else if (buffer->fd == ctx->src_buffer[index].fd
			&& buffer->index == ctx->src_buffer[index].index) {
		impp_fifo_queue(&ctx->src_full_fifo,&ctx->src_buffer[index],IMPP_WAIT_FOREVER);
	} else {
		IMPP_LOGE(TAG,"the buffer is not for  this channel");
		return -1;
	}
	return 0;
}

static int jpege_dequeue_src_buffer(void* chn_ctx,IMPP_BufferInfo_t *buffer)
{
	assert(chn_ctx);
	jpege_chn_ctx_t *ctx = (jpege_chn_ctx_t*)chn_ctx;
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

static int jpege_queue_dst_buffer(void* chn_ctx,IHAL_CodecStreamInfo_t *buffer)
{
	assert(chn_ctx);
	jpege_chn_ctx_t *ctx = (jpege_chn_ctx_t*)chn_ctx;
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

static int jpege_dequeue_dst_buffer(void* chn_ctx,IHAL_CodecStreamInfo_t *buffer)
{
	assert(chn_ctx);
	jpege_chn_ctx_t *ctx = (jpege_chn_ctx_t*)chn_ctx;
	IMPP_BufferInfo_t *buf = NULL;
	buf = impp_fifo_dequeue(&ctx->dst_full_fifo,IMPP_WAIT_FOREVER);
	if(!buf){
		IMPP_LOGE(TAG,"get src empty buffer failed");
		return -1;
	}
	buffer->fd = buf->fd;
	buffer->vaddr = buf->vaddr;
	buffer->paddr = buf->paddr;
	buffer->index = buf->index;
	buffer->size = buf->byteused;
	return 0;
}


ImppCodec_t jpegenc_v2 = {
	.name = "jpeg_enc_v2",
	.version = 2023042701,
	.codec_init = NULL,
	.codec_deinit = NULL,
	.codec_chn_ctx_create = jpege_chn_ctx_create,
	.codec_chn_ctx_destroy = jpege_chn_ctx_destroy,

	.codec_chn_start = jpege_chn_ctx_start,
	.codec_chn_stop  = jpege_chn_ctx_stop,
	.codec_param_set = jpege_param_set,
	.codec_param_get = jpege_param_get,

	.src_buffer_create = jpege_src_buffer_create,
	.src_buffer_free   = jpege_src_buffer_free,
	.dst_buffer_create = jpege_dst_buffer_create,
	.dst_buffer_free   = jpege_dst_buffer_free,

	.get_src_buffer = jpege_get_src_buffer,
	.set_src_buffer = jpege_set_src_buffer,
	.get_dst_buffer = jpege_get_dst_buffer,
	.set_dst_buffer = jpege_set_dst_buffer,

	.poll_src_buffer_avaliable = jpege_poll_src_buffer_avaliable,
	.poll_dst_buffer_avaliable = jpege_poll_dst_buffer_avaliable,

	.queue_src_buffer = jpege_queue_src_buffer,
	.dequeue_src_buffer = jpege_dequeue_src_buffer,
	.queue_dst_buffer = jpege_queue_dst_buffer,
	.dequeue_dst_buffer = jpege_dequeue_dst_buffer,

};



