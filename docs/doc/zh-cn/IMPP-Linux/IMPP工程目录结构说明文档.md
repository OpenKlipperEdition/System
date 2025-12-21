![](assets/IMPP工程目录结构说明文档.0.png)**君正®**

Copyright © Ingenic Semiconductor Co. Ltd 2023. All rights reserved.

**Release history**



| Date      | Revision | Change         |
| --------- | -------- | -------------- |
| Jan. 2023 | 1.00     | First release  |
| Jul. 2023 | 2.00     | Second release |

# 工程顶层目录结构

```
xxx@xxx:xxx/development/Ingenic-Mpp$ tree -L  2
.
├── audio										# 存放audio相关的配置文件
│   ├── asound_x2000.conf
│   ├── asound_x2500.conf
│   ├── asound_x2600.conf
│   └── webrtc_profile.ini
├── Build.mk
├── CMakeLists.txt
├── doc											# 存放Doxygen配置文件及IMPP相关的文档
│   ├── Doxyfile_cn
│   ├── IMPP工程目录结构说明文档.pdf
│   ├── IMPP开发指南.pdf
│   └── IMPP用例使用说明文档.pdf
├── include										# 存放IMPP的头文件
│   ├── audio.h
│   ├── codec.h
│   ├── display.h
│   ├── dpu.h
│   ├── drawbox.h
│   ├── icamera.h
│   ├── impp_fifo.h
│   ├── impp.h
│   ├── ipu.h
│   ├── log.h
│   └── rotater.h
├── lib											# 存放IMPP的库文件 
│   ├── x2000
│   ├── x2500
│   └── x2600
├── src											# 存放IMPP的源码及使用到的库文件
│   ├── audio									# 存放audio模块相关的源码等
│   ├── base	
│   ├── camera									# 存放camera模块相关的源码等
│   ├── CMakeLists.txt
│   ├── common									# 存放Codec模块相同的源码
│   ├── dpu										# 存放display模块及dpu模块的源码等
│   ├── external								# 存放第三方库
│   ├── log
│   ├── x2000									# 存放x2000部分模块的源码等
│   ├── x2500									# 存放x2500部分模块的源码等
│   └── x2600									# 存放x2600部分模块的源码等
└── test										# 存放IMPP的使用用例
    ├── audio_test								# audio模块相关的用例
    ├── camera-example							# camera模块相关的用例
    ├── codec-example							# Codec模块相关的用例
    ├── display-example							# Display模块和DPU模块相关的用例
    ├── img2D-example							# IPU（CSC/OSD）、Rotator及DrawBox模块相关的用例
    └── media-example							# mp4及rtsp的用例

25 directories, 22 files
```

# src目录下的结构（IMPP源码）

```
xxx@xxx:xxx/development/Ingenic-Mpp/src$ tree -L  2
.
├── audio								# 存放audio模块相关的源码等
│   ├── abuf.c
│   ├── abuf.h
│   ├── ai.c							# Audio Inupt相关的源码
│   ├── alsa							# 存放alsa的头文件
│   ├── ao.c							# Audio Output相关的源码
│   ├── asoundlib.h
│   ├── audio_dec.c						# Audio Decoder相关的源码 
│   ├── audio_enc.c						# Audio Encoder相关的源码
│   ├── audioProcess					# 存放Audio Input中Aec、Agc、Ns、Hpf相关的头文件及库文件
│   ├── audio_resampler.c				# Audio Resampler相关的源码
│   ├── codec							# 存放Audio Encoder和Audio Decoder编解码格式的源码及库文件
│   ├── libasound.a
│   └── libasound.so
├── base
│   └── impp_fifo.c
├── camera								# 存放Camera模块的源码
│   └── icamera.c
├── CMakeLists.txt
├── common								# 存放Codec模块相同的源码
│   ├── codec_common.h
│   ├── h264dec.c
│   ├── jpegd_v2.c
│   └── jpege_v2.c
├── dpu									# 存放display模块及dpu模块的源码
│   ├── dpu.c							# DPU模块相关的源码
│   ├── sample_fb.c						# Display模块相关的源码
│   └── uapi_ingenicfb.h				# DPU模块使用到的头文件
├── external							# 存放第三方库
│   ├── speexdsp						# 存放Audio Resampler使用到的库文件
│   └── webrtc							# 存放webrtc_audio_processing库的头文件
├── log
│   └── log.c
├── x2000								# 存放x2000部分模块的源码等
│   ├── CMakeLists.txt
│   ├── codec.c							# x2000 Codec模块的源码
│   └── rotater.c						# x2000 Rotater模块的源码
├── x2500								# 存放x2500部分模块的源码等
│   ├── CMakeLists.txt
│   ├── codec.c							# x2500 Codec模块的源码
│   ├── codec_libs						# 存放x2500 Codec模块使用到的库文件
│   ├── drawbox.c						# x2500 DrawBox模块的源码
│   ├── header							# 存放x2500 Codec模块使用到的库文件的头文件
│   ├── ipu.c							# x2500 IPU（CSC/OSD）模块的源码
│   ├── jz_ipu_hal.h					# x2500 IPU（CSC/OSD）模块使用到的头文件
│   └── rotater.c						# x2500 Rotater模块的源码
└── x2600								# 存放x2600部分模块的源码等
    ├── CMakeLists.txt
    └── codec.c							# x2600 Codec模块的源码

17 directories, 32 files
```
