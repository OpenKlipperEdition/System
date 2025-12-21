#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include "pwm_smc.h"

/**
 *@brief  初始化pwm_scan设备
 *@param  指向smc_context结构的smc指针，该结构包含选择的pwm通道，idel电平状态，加速步数以及匀速步数 的配置信息.
 *@retval 0 成功 非0 失败
 **/
int smc_init(struct smc_context *smc)
{
	int ret = 0;
	struct pwm_cfg_info pwm_cfg_info;

	smc->dev_fd = open(DEVICE_PATH, O_RDWR);
	if(smc->dev_fd < 0){
		perror("open dev_fd failed!!!/n");
		return -1;
	}

	ret = ioctl(smc->dev_fd, PWM_SMC_GET_INFO, &pwm_cfg_info);
	if(ret < 0){
		perror("PWM_SMC_GET_INFO operation failed!!!");
		return -1;
	}

	pwm_cfg_info.channel = smc->pwm_chan;
	pwm_cfg_info.init_level = smc->init_level;
	pwm_cfg_info.finish_level = smc->pwm_idel_level;
	pwm_cfg_info.set_up_num = smc->up;
	pwm_cfg_info.step = smc->run;
	pwm_cfg_info.period_ns = smc->period_ns;

	ret = ioctl(smc->dev_fd, PWM_SMC_SET_INFO, &pwm_cfg_info);
	if(ret < 0){
		perror("PWM_SMC_SET_INFO operation failed!!!");
		return -1;
	}

	ret = ioctl(smc->dev_fd, PWM_SMC_REQUEST_CHANNEL, NULL);
	if(ret < 0){
		perror("PWM_SMC_REQUEST_CHANNEL operation failed!!!");
		return -1;
	}

	ret = ioctl(smc->dev_fd, PWM_SMC_CONFIG, NULL);
	if(ret < 0){
		perror("PWM_SMC_CONFIG operation failed!!!");
		ret = -1;
	}

	return ret;
}

/**
 *@brief  取消初始化pwm_scan设备
 *@param  指向smc_context结构的smc指针，该结构包含选择的pwm通道，idel电平状态，加速步数以及匀速步数 的配置信息.
 *@retval 0 成功 非0 失败
 **/
int smc_deinit(struct smc_context *smc)
{
	smc->pwm_chan = 0;
	smc->pwm_idel_level = 0;
	smc->up = 0;
	smc->run = 0;
	close(smc->dev_fd);
	return 0;
}

/**
 *@brief  设置加速表，将加速表写入
 *@param  指向smc_context结构的smc指针，该结构包含选择的pwm通道，idel电平状态，加速步数以及匀速步数 的配置信息.
 *@param  需要写入的加速表
 *@retval 0 成功 非0 失败
 **/
int smc_setup_table(struct smc_context *smc, tbl_t *up)
{
	int ret = 0;

	ret = write(smc->dev_fd, up, smc->up * 4);
	if(ret != smc->up * 4){
		perror("write failed!!!\n");
		ret = -1;
	}

	return 0;
}

/**
 *@brief  启动pwm和dma
 *@param  指向smc_context结构的smc指针，该结构包含选择的pwm通道，idel电平状态，加速步数以及匀速步数 的配置信息.
 *@retval 0 成功 非0 失败
 **/
int smc_start_run(struct smc_context *smc)
{
	int ret = 0;

	ret = ioctl(smc->dev_fd, PWM_SMC_START, NULL);
	if(ret < 0){
		perror("PWM_SMC_PARM operation failed!!!");
		ret = -1;
	}

	return ret;
}

/**
 *@brief  停止数据传输并进行减速
 *@param  指向smc_context结构的smc指针，该结构包含选择的pwm通道，idel电平状态，加速步数以及匀速步数 的配置信息.
 *@retval 返回当前步数
 **/
int smc_stop_normal(struct smc_context *smc)
{
	int ret = 0;
	struct pwm_cfg_info pwm_cfg_info;

	ret = ioctl(smc->dev_fd, PWM_SMC_GEN_STOP, &pwm_cfg_info);
	if(ret < 0){
		perror("PWM_SMC_GEN_STOP operation failed!!!");
		ret = -1;
	}

	return pwm_cfg_info.cur_pos;
}

/**
 *@brief  立即停止数据传输
 *@param  指向smc_context结构的smc指针，该结构包含选择的pwm通道，idel电平状态，加速步数以及匀速步数 的配置信息.
 *@retval 返回当前步数
 **/
int smc_stop_force(struct smc_context *smc)
{
	int ret = 0;
	struct pwm_cfg_info pwm_cfg_info;
	ret = ioctl(smc->dev_fd, PWM_SMC_QCK_STOP, &pwm_cfg_info);
	if(ret < 0){
		perror("PWM_SMC_QCK_STOP operation failed!!!");
		ret = -1;
	}

	return pwm_cfg_info.cur_pos;
}

/**
 *@brief  获取当前步数
 *@param  指向smc_context结构的smc指针，该结构包含选择的pwm通道，idel电平状态，加速步数以及匀速步数 的配置信息.
 *@retval 返回当前步数
 **/
int smc_get_run_step(struct smc_context *smc)
{
	int ret = 0;
	int cur_pos = 0;
	struct pwm_cfg_info pwm_cfg_info;

	ret = ioctl(smc->dev_fd, PWM_SMC_GET_RUN_STEP_INFO, &pwm_cfg_info);
	if(ret < 0){
		perror("PWM_SMC_PARM operation failed!!!");
		ret = -1;
	}
	cur_pos = pwm_cfg_info.cur_pos;

	return cur_pos;
}



