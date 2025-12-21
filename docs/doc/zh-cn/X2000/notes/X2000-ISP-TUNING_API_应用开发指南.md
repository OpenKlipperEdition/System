![](assets/X2000-ISP-TUNING_API_应用开发指南.0.png)**君正®**


**Release history**



| Date     | Revision | Change        |
| -------- | -------- | ------------- |
| Feb.2023 | 1.00     | First release |


# ISP-TUNING简介

ISP-TUNING的主要功能是提供调节图像的亮度，锐度，饱和度，对比度，增益，曝光，宽动态等功能的API接口，便于用户自定义图像效果。

# ISP-TUNING API介绍

ISP-TUNING API 路径：
```
packages/example/App/v4l2-isp-tuning/isp_tuning_api.c
packages/example/App/v4l2-isp-tuning/include/isp_tuning_api.h
```
## 设置水平翻转

【函数原型】

```c
int isp_tuning_set_hflip(int fd, int value);
```

【功能描述】

将图像水平翻转。

【API参数说明】



| 参数  | 描述                  |
| ----- | --------------------- |
| fd    | video节点的文件描述符 |
| value | 0: disable 1: enable  |

【返回值】

成功: 0

失败: -1

【注意事项】

该函数必须在Camera开流之后进行调用。

## 获取水平状态

【函数原型】

```c
int isp_tuning_get_hflip(int fd);
```

【功能描述】

 获取当前图像水平翻转状态。

【API参数说明】



| 参数 | 描述                  |
| ---- | --------------------- |
| fd   | video节点的文件描述符 |

【返回值】

成功: 返回图像水平状态 (0:disable 1:enable)

失败: -1

【注意事项】

该函数必须在Camera开流之后进行调用。

## 设置垂直翻转

【函数原型】

```c
int isp_tuning_set_vflip(int fd, int value);
```

【功能描述】

 将图像垂直翻转。

【API参数说明】



| 参数  | 描述                  |
| ----- | --------------------- |
| fd    | video节点的文件描述符 |
| value | 0: disable 1: enable  |

【返回值】

成功: 0

失败: -1

【注意事项】

该函数必须在Camera开流之后进行调用。

## 获取垂直状态

【函数原型】

```c
int isp_tuning_get_vflip(int fd);
```

【功能描述】

 获取当前垂直翻转状态。

【参数说明】



| 参数 | 描述                  |
| ---- | --------------------- |
| fd   | video节点的文件描述符 |

【返回值】

成功: 返回图像垂直状态 (0:disable 1:enable)

失败: -1

【注意事项】

该函数必须在Camera开流之后进行调用。

## 设置锐度

【函数原型】

```c
int isp_tuning_set_sharpness(int fd, int value);
```

【功能描述】

 自定义锐度。

【参数说明】



| 参数  | 描述                  |
| ----- | --------------------- |
| fd    | video节点的文件描述符 |
| value | 锐度值 range[0-255]   |

【返回值】

成功: 0

失败: -1

【注意事项】

该函数必须在Camera开流之后进行调用。

## 获取锐度

【函数原型】

```c
 int isp_tuning_get_sharpness(int fd);
```

【功能描述】

 获取当前锐度值。

【参数说明】



| 参数 | 描述                  |
| ---- | --------------------- |
| fd   | video节点的文件描述符 |

【返回值】

成功: 返回当前锐度值

失败: -1

【注意事项】

该函数必须在Camera开流之后进行调用。

## 设置对比度

【函数原型】

```c
int isp_tuning_set_contrast(int fd, int value);
```

【功能描述】

 自定义对比度。

【参数说明】



| 参数  | 描述                  |
| ----- | --------------------- |
| fd    | video节点的文件描述符 |
| value | 对比度值 range[0-255] |

【返回值】

成功: 0

失败: -1

【注意事项】

该函数必须在Camera开流之后进行调用。

## 获取对比度

【函数原型】

```c
int isp_tuning_get_contrast(int fd);
```

【功能描述】

 获取当前对比度值。

【参数说明】



| 参数 | 描述                  |
| ---- | --------------------- |
| fd   | video节点的文件描述符 |

【返回值】

成功:返回当前对比度值

失败: -1

【注意事项】

该函数必须在Camera开流之后进行调用。

## 设置饱和度

【函数原型】

```c
int isp_tuning_set_saturation(int fd, int value);
```

【功能描述】

 自定义饱和度。

【参数说明】



| 参数  | 描述                  |
| ----- | --------------------- |
| fd    | video节点的文件描述符 |
| value | 饱和度值 range[0-255] |

【返回值】

成功: 0

失败: -1

【注意事项】

该函数必须在Camera开流之后进行调用。

## 获取饱和度

【函数原型】

```c
int isp_tuning_get_saturation(int fd);
```

【功能描述】

 获取当前饱和度值。

【参数说明】



| 参数 | 描述                  |
| ---- | --------------------- |
| fd   | video节点的文件描述符 |

【返回值】

成功: 返回当前饱和度值

失败: -1

【注意事项】

该函数必须在Camera开流之后进行调用。

## 设置亮度

【函数原型】

```c
int isp_tuning_set_brightness(int fd, int value);
```

【功能描述】

 自定义亮度值。

【参数说明】



| 参数  | 描述                  |
| ----- | --------------------- |
| fd    | video节点的文件描述符 |
| value | 亮度值 range[0-255]   |

【返回值】

成功: 0

失败: -1

【注意事项】

该函数必须在Camera开流之后进行调用。

## 获取亮度

【函数原型】

```c
int isp_tuning_get_brightness(int fd);
```

【功能描述】

 获取当前亮度值。

【参数说明】



| 参数 | 描述                  |
| ---- | --------------------- |
| fd   | video节点的文件描述符 |

【返回值】

成功: 返回当前亮度值

失败: -1

【注意事项】

该函数必须在Camera开流之后进行调用。

## 设置antiflicker

【函数原型】

```c
int isp_tuning_set_frequency(int fd, int value);
```

【功能描述】

 消除flicker。

【参数说明】



| 参数  | 描述                      |
| ----- | ------------------------- |
| fd    | video节点的文件描述符     |
| value | 0: disable 1: 50Hz2: 60Hz |

【返回值】

成功: 0

失败: -1

【注意事项】

该函数必须在Camera开流之后进行调用。

## 获取antiflicker状态

【函数原型】

```c
int isp_tuning_get_frequency(int fd);
```

【功能描述】

获取当前antiflicker状态。

【参数说明】



| 参数 | 描述                  |
| ---- | --------------------- |
| fd   | video节点的文件描述符 |

【返回值】

成功: 返回当前antiflicker状态 (0:disable 1:50Hz antiflicker 2:60Hz antiflicker)

失败: -1

【注意事项】 

该函数必须在Camera开流之后进行调用。

## 设置bypass模块

【函数原型】

```c
int isp_tuning_set_modules(int fd, int value);
```

【功能描述】

通过改写寄存器的值自定义对图像效果进行处理的模块。

【参数说明】



| 参数  | 描述                  |
| ----- | --------------------- |
| fd    | video节点的文件描述符 |
| value | 十六进制数            |

【register说明】



| Bits  | Module Name | Description            |
| ----- | ----------- | ---------------------- |
| 31:16 | Reserved    | Writing has no effect. |
| 15    | FONT        | 0: enable 1: disable   |
| 14    | TP          | 0: enable 1: disable   |
| 13    | HLDC        | 0: enable 1: disable   |
| 12    | SDNS        | 0: enable 1: disable   |
| 11    | MDNS        | 0: enable 1: disable   |
| 10    | Y_SHARPEN   | 0: enable 1: disable   |
| 9     | CLM         | 0: enable 1: disable   |
| 8     | DEFOG       | 0: enable 1: disable   |
| 7     | GAMMA       | 0: enable 1: disable   |
| 6     | CCM         | 0: enable 1: disable   |
| 5     | DMS         | 0: enable 1: disable   |
| 4     | ADR         | 0: enable 1: disable   |
| 3     | AWB         | 0: enable 1: disable   |
| 2     | LSC         | 0: enable 1: disable   |
| 1     | GIB         | 0: enable 1: disable   |
| 0     | DPC         | 0: enable 1: disable   |

【返回值】

成功: 0

失败: -1

【注意事项】 

该函数必须在Camera开流之后进行调用。

## 获取bypass信息

【函数原型】

```c
int isp_tuning_get_modules(int fd);
```

【功能描述】

获取Camera当前应用的模块bypass信息。

【参数说明】



| 参数 | 描述                  |
| ---- | --------------------- |
| fd   | video节点的文件描述符 |

【返回值】

成功: 返回一个十六进制数

失败: -1

【注意事项】 

该函数必须在Camera开流之后进行调用。

## 切换day or night

【函数原型】

```c
int isp_tuning_set_dn(int fd, int value);
```

【功能描述】

设置Camera为day mode或者night mode。

【参数说明】



| 参数  | 描述                     |
| ----- | ------------------------ |
| fd    | video节点的文件描述符    |
| value | 0: day_mode1: night_mode |

【返回值】

成功: 0

失败: -1

【注意事项】 

该函数必须在Camera开流之后进行调用。

## 获取场景mode

【函数原型】

```c
int isp_tuning_get_dn(int fd);
```

【功能描述】

获取当前Camera mode是day或者night。

【参数说明】



| 参数 | 描述                  |
| ---- | --------------------- |
| fd   | video节点的文件描述符 |

【返回值】

成功: 0或1

失败: -1

【注意事项】 

该函数必须在Camera开流之后进行调用。

## 获取luma值

【函数原型】

```c
int isp_tuning_get_luma(int fd);
```

【功能描述】

获取当前Camera 的luma值。

【参数说明】



| 参数 | 描述                  |
| ---- | --------------------- |
| fd   | video节点的文件描述符 |

【返回值】

成功: 0或1

失败: -1

【注意事项】 

该函数必须在Camera开流之后进行调用。

## 设置awb红色通道数值

【函数原型】

```c
int isp_tuning_set_wb_red(int fd, int value);
```

【功能描述】

自定义Camera 白平衡红色通道数值，范围0 - 65535。

【参数说明】



| 参数  | 描述                  |
| ----- | --------------------- |
| fd    | video节点的文件描述符 |
| value | 红色通道数值          |

【返回值】

成功: 0

失败: -1

【注意事项】 

该函数必须在Camera开流之后进行调用。

## 获取awb红色通道数值

【函数原型】

```c
int isp_tuning_get_wb_red(int fd);
```

【功能描述】

获取Camera 白平衡红色通道数值，范围0 - 65535。

【参数说明】



| 参数 | 描述                  |
| ---- | --------------------- |
| fd   | video节点的文件描述符 |

【返回值】

成功: 返回红色通道数值

失败: -1

【注意事项】 

该函数必须在Camera开流之后进行调用。

## 设置awb蓝色通道数值

【函数原型】

```c
int isp_tuning_set_wb_blue(int fd, int value);
```

【功能描述】

自定义Camera 白平衡蓝色通道数值，范围0 - 65535。

【参数说明】



| 参数  | 描述                  |
| ----- | --------------------- |
| fd    | video节点的文件描述符 |
| value | 蓝色通道数值          |

【返回值】

成功: 0

失败: -1

【注意事项】 

该函数必须在Camera开流之后进行调用。

## 获取awb蓝色通道数值

【函数原型】

```c
int isp_tuning_get_wb_blue(int fd);
```

【功能描述】

获取Camera 白平衡蓝色通道数值，范围0 - 65535。

【参数说明】



| 参数 | 描述                  |
| ---- | --------------------- |
| fd   | video节点的文件描述符 |

【返回值】

成功: 返回蓝色通道数值

失败: -1

【注意事项】 

该函数必须在Camera开流之后进行调用。

## 设置awb模式

【函数原型】

```c
int isp_tuning_set_preset_wb(int fd, int value, int red_value, int blue_value);
```

【功能描述】

自定义Camera 白平衡模式。共有11种模式，选择模式0和模式10时必须设置红色通道和蓝色通道数值，范围0 - 65535。

【参数说明】



| 参数       | 描述                       |
| ---------- | -------------------------- |
| fd         | video节点的文件描述符      |
| value      | awb mode range[0-10]       |
| red_value  | 红色通道数值range[0-65535] |
| blue_value | 蓝色通道数值range[0-65535] |

【awb mode说明】



| mode | Description                                                                                   |
| ---- | --------------------------------------------------------------------------------------------- |
| 0    | 手动白平衡                                                                                    |
| 1    | 自动白平衡                                                                                    |
| 2    | 白炽（钨丝）灯的白平衡设置。该模式通常会冷却颜色，并对应于大约2500-3500 K的色温范围。         |
| 3    | 荧光灯的白平衡预设。该模式大约对应于4000-5000 K色温。                                         |
| 4    | 使用此模式，相机将补偿荧光H照明。                                                             |
| 5    | 地平线日光的白平衡设置。该模式大约对应于5000 K色温。                                          |
| 6    | 日光（晴空）预设白平衡。该模式大约对应于5000-6500 K色温。                                     |
| 7    | 使用此模式，相机将补偿闪光灯。该模式稍微增加了暖色，大致对应于5000-5500 K的色温。（暂不支持） |
| 8    | 多云的天空预设白平衡。该模式大约对应于6500-8000 K色温范围。                                   |
| 9    | 阴影或阴天预设的白平衡。该模式大约对应9000-10000 K色温。                                      |
| 10   | 用户自定义。                                                                                  |

【返回值】

成功: 0

失败: -1

【注意事项】 

该函数必须在Camera开流之后进行调用。

## 获取awb模式

【函数原型】

```c
int isp_tuning_get_preset_wb(int fd);
```

【功能描述】

获取Camera当前应用的白平衡模式，共计10种。

【参数说明】



| 参数 | 描述                  |
| ---- | --------------------- |
| fd   | video节点的文件描述符 |

【返回值】

成功: 返回当前应用的awb模式

失败: -1

【注意事项】 

该函数必须在Camera开流之后进行调用。

## 设置自动白平衡模式

【函数原型】

```c
int isp_tuning_set_auto_wb(int fd, int value, int red_value, int blue_value);
```

【功能描述】

自定义Camera 自动白平衡模式，共计2种。选择模式0时，自动调节白平衡使用的是用户自定义的红色通道和蓝色通道数值，范围0 - 65535；选择模式1时，自动白平衡使用的是bin文件中预设的参数分量。

【参数说明】



| 参数       | 描述                       |
| ---------- | -------------------------- |
| fd         | video节点的文件描述符      |
| value      | awb mode range[0-1]        |
| red_value  | 红色通道数值range[0-65535] |
| blue_value | 蓝色通道数值range[0-65535] |

【返回值】

成功: 0

失败: -1

【注意事项】 

该函数必须在Camera开流之后进行调用。

## 获取自动白平衡模式

【函数原型】

```c
int isp_tuning_get_auto_wb(int fd);
```

【功能描述】

自定义Camera 自动白平衡模式,共计2种模式，模式0和模式1。

【参数说明】



| 参数 | 描述                  |
| ---- | --------------------- |
| fd   | video节点的文件描述符 |

【返回值】

成功: 返回0或1。

失败: -1

【注意事项】 

该函数必须在Camera开流之后进行调用。

## 设置曝光值

【函数原型】

```c
int isp_tuning_set_exp(int fd, int value);
```

【功能描述】

自定义Camera曝光。如果value = 0，自动曝光模式；如果value > 0，手动曝光模式，value即自定义的曝光值，范围0-Camera最大曝光值。（Camera最大曝光请查阅sensor手册）

【参数说明】



| 参数  | 描述                                 |
| ----- | ------------------------------------ |
| fd    | video节点的文件描述符                |
| value | 0: 自动调节曝光other: 手动设置曝光值 |

【返回值】

成功: 0

失败: -1

【注意事项】 

该函数必须在Camera开流之后进行调用。

## 获取曝光值

【函数原型】

```c
int isp_tuning_get_exp(int fd);
```

【功能描述】

获取Camera当前曝光值。

【参数说明】



| 参数 | 描述                  |
| ---- | --------------------- |
| fd   | video节点的文件描述符 |

【返回值】

成功: 返回当前曝光值

失败: -1

【注意事项】 

该函数必须在Camera开流之后进行调用。

## 设置增益值

【函数原型】

```c
int isp_tuning_set_gain(int fd, int value);
```

【功能描述】

自定义Camera增益，如果value = 0，自动增益模式；如果value > 0，手动增益模式，value即自定义的增益值，范围1-Camera最大增益值。（Camera最大增益请查阅sensor手册）

【参数说明】



| 参数  | 描述                                                    |
| ----- | ------------------------------------------------------- |
| fd    | video节点的文件描述符                                   |
| value | 0: 自动调节增益other: 手动设置增益值(1024=1x,2048=2x……) |

【返回值】

成功: 0

失败: -1

【注意事项】 

该函数必须在Camera开流之后进行调用。

## 获取增益值

【函数原型】

```c
int isp_tuning_get_gain(int fd);
```

【功能描述】

获取Camera当前增益值。

【参数说明】



| 参数 | 描述                  |
| ---- | --------------------- |
| fd   | video节点的文件描述符 |

【返回值】

成功: 返回当前增益值

失败: -1

【注意事项】 

该函数必须在Camera开流之后进行调用。

## 设置强光抑制

【函数原型】

```c
isp_tuning_set_hilightdepress(int fd, int value);
```

【功能描述】

对全局画面进行强光抑制。

【参数说明】



| 参数  | 描述                  |
| ----- | --------------------- |
| fd    | video节点的文件描述符 |
| value | 0: disable1: enable   |

【返回值】

成功: 0

失败: -1

【注意事项】 

该函数必须在Camera开流之后进行调用。

## 获取强光抑制状态

【函数原型】

```c
isp_tuning_get_hilightdepress(int fd);
```

【功能描述】

获取强光抑制状态，0未开启强光抑制，1开启强光抑制。

【参数说明】



| 参数 | 描述                  |
| ---- | --------------------- |
| fd   | video节点的文件描述符 |

【返回值】

成功: 返回0或1

失败: -1

【注意事项】 

该函数必须在Camera开流之后进行调用。

## 设置感兴趣区域

【函数原型】

```c
int isp_tuning_set_roi_ae(int fd, int enable, int top, int bottom,int left, int right);
```

【功能描述】

对感兴趣区域进行亮度调节。

【参数说明】



| 参数   | 描述                             |
| ------ | -------------------------------- |
| fd     | video节点的文件描述符            |
| enable | 0: disable1:enable               |
| top    | 指定区域的上边界（大于0）        |
| bottom | 指定区域的下边界（小于图像高度） |
| left   | 指定区域的左边界（大于0）        |
| right  | 指定区域的右边界（小于图像宽度） |

【返回值】

成功: 0

失败: -1

【注意事项】 

该函数必须在Camera开流之后进行调用，且该函数不会立即生效，需要循环调用。

