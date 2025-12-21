#ifndef __PWM_SMC__H__
#define __PWM_SMC__H__

#define DEVICE_PATH "/dev/ingenic_pwm_smc"

typedef struct {
	short Low;
	short High;
} tbl_t;

struct pwm_cfg_info {
	int mode;		/*pwm的模式，当前配置为DMA_MODE_SMC*/
	int init_level;		/*init电平*/
	int finish_level;	/*idel电平*/
	int period_ns;		/*周期时间*/
	int duty_ns;		/*由于配置占空比*/
	int channel;		/*使用的pwm通道*/
	int clk_in;		/*pwm时钟*/
	int step;		/*匀速的步数*/
	int set_up_num;		/*加速的步数*/
	int cur_pos;		/*当前步数*/
};

enum pwm_scan_ioctl_cmd {
	PWM_SMC_GET_INFO,		/*获取结构体数据*/
	PWM_SMC_SET_INFO,		/*设置结构体数据*/
	PWM_SMC_CONFIG,			/*初始化pwm和dma*/
	PWM_SMC_START,			/*启动pwm和dma*/
	PWM_SMC_GEN_STOP,		/*缓停，返回当前步数*/
	PWM_SMC_QCK_STOP,		/*急停，返回当前步数*/
	PWM_SMC_GET_RUN_STEP_INFO,	/*获取当前步数*/
	PWM_SMC_REQUEST_CHANNEL,	/*申请pwm通道*/
};

struct smc_context {
	int dev_fd;		/*设备id*/
	int pwm_chan;		/*使用的pwm通道*/
	int pwm_idel_level;	/*idel电平*/
	int init_level;		/*init电平*/
	int up;			/*加速表的步数*/
	int run;		/*匀速的步数*/
	int period_ns;		/*周期时间*/
};


int smc_init(struct smc_context *smc);
int smc_deinit(struct smc_context *smc);
int smc_setup_table(struct smc_context *smc, tbl_t *up);
int smc_start_run(struct smc_context *smc);
int smc_stop_normal(struct smc_context *smc);
int smc_stop_force(struct smc_context *smc);
int smc_run_step(struct smc_context *smc);
#endif

