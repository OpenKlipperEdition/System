# GPIO测试介绍
GPIO提供的API, 是封装/sys/class/gpio/接口的功能。利用API的前提是，空闲GPIO。  
在 /sys/devices/platform/apb/10010000.pinctrl/dump_gpio,用户可以 cat dump_gpio 文件,查看对应 gpio function 功能是否为期望功能,以达到debug 的目的。  
设备数中有的GPIO作为作为其他外设的中断引脚等等，即被使用的GPIO不可利用API操作，但
当然可以利用API使用作为function功能的引脚，但不推荐，可能会造成驱动中使用该引脚的模块出现问题。
## 简介

本测试主要用来测试GPIO输出功能和输入中断功能

## 目录结构
```
├── ihal_config.h           # 自定义配置文件
├── Makefile                # 编译文件
├── README.md               # 说明文件
└── gpio_test.c             # 测试文件 (输出功能、输入中断功能)

```
## 测试自定义配置

在 ***ihal_config.h*** 根据不同的板级条件编译,自定义配置GPIO引脚信息：GROUP和PIN参数。文件默认测试GROUP和PIN参数已定义，如需修改，按需修改。
例如x2000：
```
#elif defined X2000_HALLEY5
#define GROUP_OUT           GPIOA   /* 端口名称，如GPIOA、GPIOB 范围：GPIOA~E */
#define PIN_OUT             14      /* 端口编号 如0、1          范围：0 ~ 31 */
#define GROUP_Interrupt     GPIOE
#define PIN_Interrupt       31

```
## 测试流程
### 输出功能
1. 初始化GPIO
2. 设置输出模式
3. 设置高电平
4. 读取电平值
5. 设置反向极性
6. 读取电平值
7. 设置正向极性(恢复状态)
8. 反初始化GPIO(释放资源)
### 测试结果
引脚输出相应电平，终端log打印:  
affer set high level, gpio value : 1   
after toggle gpio, gpio value : 0   
after set reversed polarity:GPIO_ACTIVE_HIGH, get gpio value : 1      
after restore normal polarity:GPIO_ACTIVE_LOW, get gpio value : 0    
#### 输入中断功能 
1. 初始化GPIO
2. 设置输入模式
3. 设置中断模式（下降沿）
4. 开启中断探测（同时定义中断处理函数）
6. 反初始化GPIO(释放资源)
### 测试结果
x1600、x2000每按下 WKUP 按键 / x2500、x26xx系列每按下 BOOT_SEL1 按键， 触发执行中断处理函数，终端log打印:   
GPIO Interrupt Occurred！  
## 测试准备和注意事项
```
使用API的GPIO引脚的选择应是空闲GPIO

输出功能引脚：
    ihal_config.h文件已经默认定义
输入中断功能引脚：
    中断按键引脚，ihal_config.h文件已经默认定义
    x1600 、X2000：修改板级设备树，取消设备树中wakeup节点
    注：如板级设备数没有，不用注释

gpio_keys: gpio_keys {
            compatible = "gpio-keys";

         /* wakeup {
                label = "WAKEUP";
                linux,code = <KEY_WAKEUP>;        
                gpios = <&gpe 31 GPIO_ACTIVE_LOW INGENIC_GPIO_NOBIAS>;
                gpio-key,wakeup;
            };
         */
            bootsel0 {
                label = "bootsel0";
                linux,code = <KEY_HOME>;
                gpios = <&gpe 25 GPIO_ACTIVE_HIGH INGENIC_GPIO_NOBIAS>;
            };
            .............

    x2500 、x26xx系列：修改板级设备树，取消设备树中bootsel1节点
    注：如板级设备数没有,不用注释
     gpio_keys: gpio_keys {
            compatible = "gpio-keys";
 
           /* bootsel1 {
                label = "bootsel1";
                linux,code = <KEY_BACK>;
                gpios = <&gpd 15 GPIO_ACTIVE_LOW INGENIC_GPIO_NOBIAS>;
                gpio-key,wakeup;
            };
           */ 
            .............

```