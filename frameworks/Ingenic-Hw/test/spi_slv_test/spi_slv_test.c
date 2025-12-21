#include <stdio.h>
#include <string.h>
#include "ihal_config.h"
#include "ihal.h"

#define TRANSFER_LEN 512

int t_test(IHal_SPI_SLV_Handle_t *handle, char *data_buf)
{
	int ret = 0;
	char send_buf[TRANSFER_LEN] = {0};
	memcpy(send_buf, data_buf, TRANSFER_LEN);
	ret = IHal_SPI_SLV_SendData(handle, send_buf, TRANSFER_LEN);
	if (ret != 0) {
		printf("Failed to send data!\n");
		return -1;
	}
	printf("Send ok!\n");
	return 0;
}

int r_test(IHal_SPI_SLV_Handle_t *handle, char *data_buf)
{
	int i = 0;
	int ret = 0;
	char receiv_buf[TRANSFER_LEN] = {0};
	ret = IHal_SPI_SLV_ReceivData(handle, receiv_buf, TRANSFER_LEN);
	if (ret != 0) {
		printf("Failed to recv data!\n");
		return -1;
	}
	ret = 0;
	for (i = 0; i < TRANSFER_LEN; i++) {
		if(receiv_buf[i] != data_buf[i]){
			ret = -1;
		}
	}
	if(ret == 0){
		printf("Recv ok!\n");
	}else{
		printf("Recv data error!\n");
	}
	return 0;
}

int tr_test(IHal_SPI_SLV_Handle_t *handle, char *data_buf)
{
	int i = 0;
	int ret = 0;
	char send_buf[TRANSFER_LEN] = {0};
	char receiv_buf[TRANSFER_LEN] = {0};
	memcpy(send_buf, data_buf, TRANSFER_LEN);
	ret = IHal_SPI_SLV_Transfer(handle, send_buf, receiv_buf, TRANSFER_LEN);
	if (ret != 0) {
		printf("Failed to tr data!\n");
		return -1;
	}
	ret = 0;
	for (i = 0; i < TRANSFER_LEN; i++) {
		if(receiv_buf[i] != data_buf[i]){
			ret = -1;
		}
	}
	if(ret == 0){
		printf("TR ok!\n");
	}else{
		printf("Recv data error!\n");
	}
	return 0;
}

int main(int argc, char *argv[])
{
	if(argc != 2){
		printf("Please select transfer dirction(t | r | tr)!\n");
		return -1;
	}
	char node[DEV_NODE_PATH_LEN] = "/dev/spi_slv";
	IHal_SPI_SLV_attr attr;
	attr.dev_node = node;
	attr.spi_slv_mode = SPI_MODE3;
	attr.can_dma = DMA_MODE;
	attr.bits_per_word = 8;

	IHal_SPI_SLV_Handle_t *handle = NULL;
	handle = IHal_SPI_SLV_Init(&attr);
	if (handle == NULL) {
		printf("SPI_SLV Init error!\n");
		return -1;
	}
	int i = 0;
	int ret = 0;
	char j = '0';
	char data_buf[TRANSFER_LEN] = {0};
	for (i = 0; i < TRANSFER_LEN; i++) {
		data_buf[i] = j;
		j++;
		if ((i + 1) % 10 == 0) {
			j = '0';
		}
	}
	if(!strcmp(argv[1], "t")){
		t_test(handle, data_buf);
	}else if(!strcmp(argv[1], "r")){
		r_test(handle, data_buf);
	}else if(!strcmp(argv[1], "tr")){
		tr_test(handle, data_buf);
	}else{
		printf("The argv[1] is error, please input (t | r | tr)\n");
		return -1;
	}
	ret = IHal_SPI_SLV_DeInit(handle);
	if (ret != 0) {
		printf("Failed to DeInit spi_slv!\n");
		return -1;
	}
	return 0;
}
