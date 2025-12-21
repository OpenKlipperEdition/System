# UART测试介绍

## 简介

本测试用例以x2600e为例，主要用来测试串口的基本功能是否正常

## 目录结构

```
.
├── ihal_config.h     # 自定义配置文件
├── Makefile          # 编译文件
├── uart_test.c        # cpu模式测试文件
├── uart_test-dma.c    # dma模式测试文件
└── README.md		  # 说明文件

```
## 测试自定义配置
```
使用前需要先找到两个空闲的UART,(两个串口的电压必须相同，都为3.3v或者都为1.8v)。例如：UART4、UART5,将UART4的RX_D引脚与UART5的TX_D引脚连接，UART4的TX_D引脚与UART5的RX_D引脚连接后，修改设备树的配置，如下：

板级设备树位置:kernel/kernel-版本/module_drivers/dts/

&uart4 {
        status = "okay";
        pinctrl-names = "default";
        pinctrl-0 = <&uart4_px>;    //x取决于你所选取的引脚，比如我选择的就是b
                                    //组的gpio x则为b。
        /*dma-mode;*/               //没有dma-mode节点则uart处于正常工作模式
                                    //如果添加上dma-mode节点则uart处于DMA模
                                    //根据需求自行修改 。  
};

&uart5 {
        status = "okay";
        pinctrl-names = "default";
        pinctrl-0 = <&uart5_px>;
        /*dma-mode;*/
};



注：以上是设备树的具备配置，根据需求自行修改。
```
## 测试流程
```
1. 初始化串口(打开设备节点、设置默认输出方式为波特率115200，"8n1"、设置一些其他必需的配置)
2. 更改UART配置(根据需要更改变串口的波特率，数据位，停止位，奇偶校验位等参数)
3. 发送数据
4. 接收数据
5. 反初始化

## 测试结果

接收到串口发送来的消息 "hello beijing-ingenic!!!!"

注意事项：
1.要关注gpio的引脚复用和初始电平，确保你选择的引脚为UART的复用功能，并且初始电平为HIGH;
2.本测试用例不能做到自动化，需要用户手动配置设备树，以及参考电路图选取UART。