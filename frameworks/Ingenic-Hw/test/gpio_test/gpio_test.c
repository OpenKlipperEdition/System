/**
 * @file    gpio_test.c
 * @author  MPU系统软件部团队
 * @brief   包含 GPIO HAL 库函数gpio_test测试的源文件
 *
 * @copyright 版权所有 (北京君正集成电路股份有限公司) {2023}
 * @copyright Copyright© 2023 Ingenic Semiconductor Co.,Ltd
 *
 * This program is free software; you can redistribute it and/or modify it
 * under  the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "ihal_config.h"
#include "ihal.h"

/*************** 输出功能 *****************/
void *function_output(void *arg)
{
    int ret = -1;
    //初始化GPIO
    IHAL_GPIO_Handle_t *gpio_handle = IHal_GPIO_Init(GROUP_OUT, PIN_OUT);
    if (!gpio_handle) {
        printf("Init gpio fail!\n");
    }
    // 设置输出
    ret = IHal_GPIO_Set_Direciton(gpio_handle, GPIO_OUT);
    if (ret < 0) {
        printf("set out direction fail!\n");
        goto deinit;
    }
    //输出高电平
    ret = IHal_GPIO_Set_Value(gpio_handle, GPIO_HIGH);
    if (ret < 0) {
        printf("set high value fail!\n");
        goto deinit;
    }
    //获取电平
    ret = IHal_GPIO_Get_Value(gpio_handle);
    if (ret < 0) {
        printf("get gpio value fail!\n");
        goto deinit;
    }
    //打印电平
    printf("affer set high level, gpio value : %d\n", ret);

    //翻转电平后获取电平
    ret = IHal_GPIO_Toggle_Value(gpio_handle);
    if (ret < 0) {
        printf("toggle gpio fail\n");
        goto deinit;
    }

    ret = IHal_GPIO_Get_Value(gpio_handle);
    if (ret < 0) {
        printf("after toggle gpio,get gpio value fail!\n");
        goto deinit;
    }
    //打印电平
    printf("after toggle gpio, gpio value : %d\n", ret);

    //设置反向极性后，获取电平 (一般无需操作，GPIO默认正向极性-GPIO_ACTIVE_LOW)
    ret = IHal_GPIO_Set_Polarity(gpio_handle, GPIO_ACTIVE_HIGH);
    if (ret < 0) {
        printf("set GPIO_ACTIVE_HIGH fail!\n");
        goto deinit;
    }
    ret = IHal_GPIO_Get_Value(gpio_handle);
    if (ret < 0) {
        printf("after set reversed polarity:GPIO_ACTIVE_HIGH, get gpio value fail!\n");
        goto deinit;
    }
    //打印电平
    printf("after set reversed polarity:GPIO_ACTIVE_HIGH, get gpio value : %d\n", ret);

    //恢复正常极性
    ret = IHal_GPIO_Set_Polarity(gpio_handle, GPIO_ACTIVE_LOW);
    if (ret < 0) {
        printf("restore normal polarity:GPIO_ACTIVE_LOW fail\n");
        goto deinit;
    }
    ret = IHal_GPIO_Get_Value(gpio_handle);
    if (ret < 0) {
        printf("after restore normal polarity:GPIO_ACTIVE_LOW, get gpio value fail!\n");
        goto deinit;
    }
    //打印电平
    printf("after restore normal polarity:GPIO_ACTIVE_LOW, get gpio value : %d\n", ret);

    //反初始化 回收资源
    ret = IHal_GPIO_DeInit(gpio_handle);
    if (ret < 0) {
        printf("DeInit fail\n");
    }
    return NULL;
deinit:
    ret = IHal_GPIO_DeInit(gpio_handle);
    if (ret < 0) {
        printf("DeInit fail\n");
    }
    return NULL;
}

//在这里编写中断处理代码
void interrupt_handler()
{
    static int a = 1;
    printf("GPIO Interrupt Occurred%d\n", a);
    a++;
}
//中断探测线程
void *interrupt_detect(void *arg)
{
    IHAL_GPIO_Handle_t *gpio_handle = (IHAL_GPIO_Handle_t *)arg;
    //中断检测，该函数会等待中断信号，如果触发一次，则回调一次interrupt_handler函数。最后参数设置触发次数，ALWAYS(一直触发)
    IHal_GPIO_Interrupt_Detect(gpio_handle, interrupt_handler, ALWAYS);

    return NULL;
}
/*************** 输入中断功能 *****************/
void *function_interrupt_input(void *arg)
{
    int ret = -1;
    //初始化gpio
    IHAL_GPIO_Handle_t *gpio_handle = IHal_GPIO_Init(GROUP_Interrupt, PIN_Interrupt);
    if (!gpio_handle) {
        printf("Init gpio fail!\n");
    }
    //输入
    ret = IHal_GPIO_Set_Direciton(gpio_handle, GPIO_IN);
    if (ret < 0) {
        printf("set in direction fail!\n");
        goto deinit;
    }
    //下降沿中断
    ret = IHal_GPIO_Set_Interrupt_mode(gpio_handle, GPIO_FALLING); //必须设置触发方式
    if (ret < 0) {
        printf("set falling intrrrupt fail!\n");
        goto deinit;
    }
    //创建一个线程，中断触发探测。这样该线程可以继续往下进行其他处理。创建的线程处理gpio中断，如果触发，则处理中断
    pthread_t thread3;
    pthread_create(&thread3, NULL, interrupt_detect, gpio_handle);
    /*
        在此之间可以处理其他任务


    */
    pthread_join(thread3, NULL);
    //反初始化 回收资源
    ret = IHal_GPIO_DeInit(gpio_handle);
    if (ret < 0) {
        printf("DeInit fail\n");
    }
deinit:
    ret = IHal_GPIO_DeInit(gpio_handle);
    if (ret < 0) {
        printf("DeInit fail\n");
    }
    return NULL;
}
//主线程结束时，进程会被终止，从而导致所有子线程的执行也被终止
int main()
{
    pthread_t thread1;
    pthread_t thread2;
    pthread_create(&thread1, NULL, function_output, NULL);
    pthread_create(&thread2, NULL, function_interrupt_input, NULL);

    pthread_join(thread1, NULL);
    pthread_join(thread2, NULL);
}