#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>
#include "pwm_smc.h"

int main(int argc, char** argv)
{
	int ret = 0;
	int err;
	pthread_t id;
	int run_steps = 1000;
	int period_ns = 10000;
	struct smc_context smc;
	tbl_t pwm_form_high[50];
	struct pwm_cfg_info pwm_cfg_info;
	int i = 0;
	for(i = 0; i < 50; i++){
		pwm_form_high[i].High = 40*(i+1);
		pwm_form_high[i].Low = 4000 - 40 * (i+1);
	}

	smc.pwm_chan = 12;
	smc.pwm_idel_level = 0;
	smc.init_level = 0;
	smc.up = sizeof(pwm_form_high) / 4;
	smc.run = run_steps;
	smc.period_ns = period_ns;

	ret = smc_init(&smc);
	if(ret < 0){
		perror("set pwm_run_steps failed !!!");
		return -1;
	}
	ret = smc_setup_table(&smc, pwm_form_high);
	if(ret < 0){
		perror("smc_setup_table failed !!!");
		ret = -1;
	}

	int pages = 10;
	int cnt = 0;
	int total_step = smc.up * 2 + smc.run;
	for(i = 0; i < pages; i++){
		ret = smc_start_run(&smc);
		if(ret < 0){
			perror("smc_start_run failed !!!");
			ret = -1;
		}
		while(ret < total_step){
			ret = smc_get_run_step(&smc);
		}
			printf("count = %d\n",ret);
	}

	ret = smc_start_run(&smc);
	usleep(200);
	//stop test
	ret = smc_get_run_step(&smc);
	if(ret < smc.up)
		ret = smc_stop_force(&smc);
	else
		ret = smc_stop_normal(&smc);
	printf("ret = %d\n",ret);
	usleep(150);
	ret = smc_get_run_step(&smc);
	printf("ret = %d\n",ret);
	smc_deinit(&smc);
	return 0;
}


