# I2D图像旋转模块

I2D图像旋转模块支持对图像进行旋转，上下镜像以及左右镜像的功能；



|  |  |  |
| --- | --- | --- |
| module | Input/Output Format | Output Destination |
| I2D | NV12/RAW8/RGB565/ARGB8888 | DDR |

格式说明：该模块不具备格式转换功能，输入格式和输出格式是一致的。

## 驱动源码位置

module_drivers/drivers/media/platform/ingenic-i2d

## 驱动配置

Symbol: INGENIC_I2D [=y] 

Type : tristate 

Prompt: Ingenic I2D Driver 

 Location: 

 -> Ingenic device-drivers Configurations 

 -> [I2D] Drivers 

 -> JZ I2D Driver (JZ_I2D [=y]) 

 Defined at module_drivers/drivers/media/platform/ingenic-i2d/Kconfig 

 

## 设备节点

设备加载成功后默认生成驱动节点为：

/dev/video0

## 示例程序

参考IMPP库中的I2Dr图像旋转示例


