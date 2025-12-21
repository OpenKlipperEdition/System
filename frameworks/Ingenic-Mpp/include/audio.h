/**
 * @file audio.h
 * @author ingenic-team
 * @brief
 * @version 0.1
 * @date 2021-12-06
 *
 * @copyright Copyright (c) ingenic 2021
 *
 */

#ifndef __AUDIO_H__
#define __AUDIO_H__

#include "impp.h"

/**
 * @defgroup group_Audio Aduio模块
 * @{
 */

/**
 * @addtogroup group_Audio_data_type 数据类型定义
 * @{
 */

/**
 * @brief 单声道类型
 */
typedef enum {
        MONO_LEFT,                                                      /*!< 左声道           */
        MONO_RIGHT,                                                     /*!< 右声道           */
} IHal_Audio_MonoType_t;

/**
 * @brief Audio type
 */
typedef enum {
        IHal_AudioCapture,                                      /*!< 录音              */
        IHal_AudioPlayback,                                     /*!< 放音              */
} IHal_AudioType;

/**
 * @brief Audio AGC配置
 */
typedef struct {
        int DbLevel;                                            /*!< 增益级别[0 31] ,值越小，音量越大*/
        int MaxGainDb;                                          /*!< 最大增益值[0 90],值越大，增益越大*/
} IHal_AudioAgcConfig_t;

/**
 * @brief 音频编解码格式
 */
typedef enum {
        Codec_G711A,
        Codec_G711U,
        Codec_G726,
        Codec_ADPCM,
        Codec_AAC_ADTS,
        Codec_AAC_ADIF,
        Codec_AAC_RAW,
} IHal_AudioCodecType_t;

/**
 * @brief Audio handle
 */
struct IHal_AudioHandle {
        IHal_AudioType type;
        void *pcm_handle;
        void *priv;
};

/**
 * @brief 录音通道的属性
 */
typedef struct {
        char *audio_node;                                       /*!< PCM设备名字           */
        IMPP_SAMPLE_FMT_t SampleFmt;            /*!< 采样格式              */
        IHAL_UINT32 SampleRate;                         /*!< 采样频率              */
        IHAL_UINT32 numPerSample;                       /*!< 一帧数据包含的采样数  */
        IHAL_UINT32 channels;                           /*!< 声道数                */
        IHAL_INT32 bufferDeep;                          /*!< 缓冲区个数            */
} IHal_AI_Attr_t;

/**
 * @brief 放音通道的属性
 */
typedef struct {
        char *audio_node;                                       /*!< PCM设备名字           */
        IMPP_SAMPLE_FMT_t SampleFmt;            /*!< 采样格式              */
        IHAL_UINT32 SampleRate;                         /*!< 采样频率              */
        IHAL_UINT32 numPerSample;                       /*!< 一帧数据包含的采样数  */
        IHAL_UINT32 channels;                           /*!< 声道数                */
        IHAL_INT32 bufferDeep;                          /*!< 缓冲区个数            */
} IHal_AO_Attr_t;

/**
 * @brief 音频帧信息
 */
typedef struct {
        IHAL_UINT32 *vaddr;                                     /*!< 虚拟地址              */
        IHAL_UINT32  datalen;                           /*!< 长度                  */
} IHal_AudioFrm_t;

/**
 * @brief 音频数据流信息
 */
typedef struct {
        IHAL_UINT32 *vaddr;                                     /*!< 虚拟地址             */
        IHAL_UINT32  size;                                      /*!< 音频数据流大小       */
        IHAL_UINT64 timestamp;                          /*!< 时间戳                            */
        IHAL_UINT64  seq;                                       /*!< 序列号               */
        void        *node;
} IHal_AudioStream_t;

/**
 * @brief 音频数据缓冲区
 */
typedef struct {
        IHAL_UINT32 *vaddr;                                     /*!< 虚拟地址             */
        IHAL_INT32  datalen;                            /*!< 数据长度             */
        IHAL_UINT64 timestamp;                          /*!< 时间戳                            */
        IHAL_UINT64  seq;                                       /*!< 序列号               */
        void *node;
} IHal_AudioBuffer_t;

typedef struct IHal_AudioHandle IHal_AudioHandle_t;

/**
 * @brief 音频编码器的属性
 */
typedef struct {
        IHal_AudioCodecType_t type;                             /*!< 音频编码格式          */
        IMPP_SAMPLE_FMT_t SampleFmt;                    /*!< 源PCM数据格式         */
        IHAL_UINT32           SampleRate;               /*!< 采样率　　　　        */
        IHAL_UINT32                       BitRate;                      /*!< 码率                    */
        IHAL_UINT32                       Quality;                      /*!< 音质                            */
        IHAL_UINT32           Channels;                 /*!< 声道数                */
        IHAL_UINT32           numPerSample;             /*!< 一帧数据包含的采样数  */
        IHAL_UINT32           bufsize;                  /*!< 缓冲区个数            */
} IHal_AudioEnc_Attr_t;

/**
 * @brief 音频解码器的属性
 */
typedef struct {
        IHal_AudioCodecType_t type;                             /*!< 音频解码格式                */
        IHAL_UINT32                       BitRate;                      /*!< 码率                          */
        IHAL_UINT32           SampleRate;       /*!< 采样率　　　　　　　　　　　　*/
        IHAL_UINT32           AO_numPerSample;  /*!< 同放音的numPerSample保持一致  */
        IHAL_INT32            Channels;                 /*!< 声道数                        */
        IHAL_UINT32           bufsize;                  /*!< 缓冲区个数                    */
} IHal_AudioDec_Attr_t;

/**
 * @brief 重采样属性
 */
typedef struct {
        IHAL_UINT32 InSamplerate;                               /*!< 源数据采样率                          */
        IHAL_UINT32 OutSamplerate;                              /*!< 输出数据采样率                               */
        IHAL_INT32  Channels;                                   /*!< 通道数量                                        */
        IHAL_INT32  Quality;                                    /*!< 转换质量 范围:[0 - 10]                */
        IHAL_INT32  MaxNumPersamples;                   /*!< 单次最大处理samples       */
} IHal_ResamplerAttr_t;

typedef void IHal_AudioEnc_Handle_t;

typedef void IHal_AudioDec_Handle_t;

typedef void IHal_Resampler_Handle_t;
/**
 * @}
 */

/**
 * @addtogroup group_Audio_API API定义
 * @{
 */

/******************************* Audio Input ******************************/
/**
 * @brief 创建一个音频输入通道
 * @param [in] attr : 录音通道的属性
 * @retval IHal_AudioHandle_t   成功
 * @retval IHAL_RNULL                   失败
 */
IHal_AudioHandle_t *IHal_AI_ChanCreate(IHal_AI_Attr_t *attr);

/**
 * @brief 销毁一个音频输入通道
 * @param [in] handle : Audio handle
 * @retval 0    成功
 * @retval 非0 失败
 */
IHAL_INT32 IHal_AI_ChanDestroy(IHal_AudioHandle_t *handle);

/**
 * @brief 启动音频输入通道
 * @param [in] handle : Audio handle
 * @retval 0    成功
 * @retval 非0 失败
 */
IHAL_INT32 IHal_AI_ChanStart(IHal_AudioHandle_t *handle);

/**
 * @brief 停止音频输入通道
 * @param [in] handle : Audio handle
 * @retval 0    成功
 * @retval 非0 失败
 */
IHAL_INT32 IHal_AI_ChanStop(IHal_AudioHandle_t *handle);

/**
 * @brief 获取一帧音频输入数据
 * @param [in]  handle : Audio handle
 * @param [out] buf        : 音频数据缓冲区
 * @param [in]  wait   : 等待类型（ #IMPP_NO_WAIT 或 #IMPP_WAIT_FOREVER ）
 * @retval 0    成功
 * @retval 非0 失败
 */
IHAL_INT32 IHal_AI_GetBuffer(IHal_AudioHandle_t *handle, IHal_AudioBuffer_t *buf, IHAL_INT32 wait);

/**
 * @brief 释放音频输入缓冲区
 * @param [in] handle : Audio handle
 * @param [in] buf        : 音频数据缓冲区
 * @retval 0    成功
 * @retval 非0  失败
 */
IHAL_INT32 IHal_AI_ReleaseBuffer(IHal_AudioHandle_t *handle, IHal_AudioBuffer_t *buf);

/**
 * @brief 设置录音音量
 * @param [in] handle : Audio handle
 * @param [in] value  : 音量值（0 - 100）
 * @retval 0    成功
 * @retval 非0 失败
 */
IHAL_INT32 IHal_AI_SetVolume(IHal_AudioHandle_t *handle, IHAL_INT32 value);

/**
 * @brief 获取录音音量
 * @param [in]  handle : Audio handle
 * @param [out] value  : 音量值（0 - 100）
 * @retval 0    成功
 * @retval 非0 失败
 */
IHAL_INT32 IHal_AI_GetVolume(IHal_AudioHandle_t *handle, IHAL_INT32 *value);

/**
 * @brief 设置录音Gain
 * @param [in] handle : Audio handle
 * @param [in] value  : 增益值
 * @retval 0    成功
 * @retval 非0 失败
 */
IHAL_INT32 IHal_AI_SetGain(IHal_AudioHandle_t *handle, IHAL_INT32 value);

/**
 * @brief 获取录音Gain
 * @param [in]  handle : Audio handle
 * @param [out] value  : 增益值
 * @retval 0    成功
 * @retval 非0  失败
 */
IHAL_INT32 IHal_AI_GetGain(IHal_AudioHandle_t *handle, IHAL_INT32 *value);

/**
 * @brief 使能回声消除
 * @param [in] handle : Audio handle
 * @retval 0    成功
 * @retval 非0 失败
 */
IHAL_INT32 IHal_AI_EnableAec(IHal_AudioHandle_t *handle);

/**
 * @brief 禁用回声消除
 * @param [in] handle : Audio handle
 * @retval 0    成功
 * @retval 非0 失败
 */
IHAL_INT32 IHal_AI_DisableAec(IHal_AudioHandle_t *handle);

/**
 * @brief 设置音频输入通道的单声道类型
 * @param [in] handle : Audio handle
 * @param [in] mono   : 单声道类型
 * @retval 0    成功
 * @retval 非0  失败
 */
IHAL_INT32 IHal_AI_SingleChannelSet(IHal_AudioHandle_t *handle, IHal_Audio_MonoType_t mono);

/**
 * @brief 使能自动增益
 * @param [in] handle : Audio handle
 * @param [in] config : agc config
 * @retval 0    成功
 * @retval 非0  失败
 */
IHAL_INT32 IHal_AI_EnableAgc(IHal_AudioHandle_t *handle, IHal_AudioAgcConfig_t *config);

/**
 * @brief 关闭自动增益
 * @param [in] handle : Audio handle
 * @retval 0    成功
 * @retval 非0  失败
 */
IHAL_INT32 IHal_AI_DisableAgc(IHal_AudioHandle_t *handle);

/**
 * @brief 使能降噪
 * @param [in] handle : Audio handle
 * @mode  [in] mode   : mode [0,3]
 * @retval 0    成功
 * @retval 非0  失败
 */
IHAL_INT32 IHal_AI_EnableNs(IHal_AudioHandle_t *handle, int mode);

/**
 * @brief 关闭降噪
 * @param [in] handle : Audio handle
 * @mode  [in] mode   : mode [0,3]
 * @retval 0    成功
 * @retval 非0  失败
 */
IHAL_INT32 IHal_AI_DisableNs(IHal_AudioHandle_t *handle);

/**
 * @brief 使能高通滤波
 * @param [in] handle : Audio handle
 * @param [in] cutoff_freq : HPF cut off frequency
 * @retval 0    成功
 * @retval 非0  失败
 */
IHAL_INT32 IHal_AI_EnableHpf(IHal_AudioHandle_t *handle, unsigned int cutoff_Freq);

/**
 * @brief 关闭高通滤波
 * @param [in] handle : Audio handle
 * @retval 0    成功
 * @retval 非0  失败
 */
IHAL_INT32 IHal_AI_DisableHpf(IHal_AudioHandle_t *handle);

/******************************* Audio Output ******************************/
/**
 * @brief 创建一个音频输出通道
 * @param [in] attr : 放音通道的属性
 * @retval IHal_AudioHandle_t 成功
 * @retval IHAL_RNULL             失败
 */
IHal_AudioHandle_t *IHal_AO_ChanCreate(IHal_AO_Attr_t *attr);

/**
 * @brief 销毁一个音频输出通道
 * @param [in] handle : Audio handle
 * @retval 0    成功
 * @retval 非0 失败
 */
IHAL_INT32 IHal_AO_ChanDestroy(IHal_AudioHandle_t *handle);

/**
 * @brief 向音频输出通道写入一帧数据
 * @param [in] handle    : Audio handle
 * @param [in] frame     : 写入的数据帧
 * @param [in] waitblock : 等待类型（ #IMPP_NO_WAIT 或 #IMPP_WAIT_FOREVER ）
 * @retval 0    成功
 * @retval 非0 失败
 */
IHAL_INT32 IHal_AO_ChanWriteData(IHal_AudioHandle_t *handle, IHal_AudioFrm_t *frame, IHAL_INT32 waitblock);

/**
 * @brief 启动音频输出通道
 * @param [in] handle : Audio handle
 * @retval 0    成功
 * @retval 非0 失败
 */
IHAL_INT32 IHal_AO_ChanStart(IHal_AudioHandle_t *handle);

/**
 * @brief 停止音频输出通道
 * @param [in] handle : Audio handle
 * @retval 0    成功
 * @retval 非0  失败
 */
IHAL_INT32 IHal_AO_ChanStop(IHal_AudioHandle_t *handle);

/**
 * @brief 重启频输出通道:立即终止当前播放状态并清空缓冲区
 * @param [in] handle : Audio handle
 * @retval 0    成功
 * @retval 非0  失败
 */
IHAL_INT32 IHal_AO_ChanReStart(IHal_AudioHandle_t *handle);

/**
 * @brief 刷新音频输出通道缓冲区中的数据，防止缺失数据，可在停止播放前调用
 * @param [in] handle : Audio handle
 * @retval 0    成功
 * @retval 非0  失败
 */
IHAL_INT32 IHal_AO_BufferFlush(IHal_AudioHandle_t *handle);

/**
 * @brief 暂停音频播放
 * @param [in] handle : Audio handle
 * @retval 0    成功
 * @retval 非0  失败
 */
IHAL_INT32 IHal_AO_Pause(IHal_AudioHandle_t *handle);

/**
 * @brief 恢复音频播放
 * @param [in] handle : Audio handle
 * @retval 0    成功
 * @retval 非0  失败
 */
IHAL_INT32 IHal_AO_Resume(IHal_AudioHandle_t *handle);

/**
 * @brief 设置静音播放状态
 * @param [in] handle : Audio handle
 * @param [in] mute   : 静音状态
 * @retval 0    成功
 * @retval 非0  失败
 */
IHAL_INT32 IHal_AO_SetMute(IHal_AudioHandle_t *handle, IHAL_INT32 mute);

/**
 * @brief 获取静音播放状态
 * @param [in]  handle : Audio handle
 * @param [out] mute   : 静音状态
 * @retval 0    成功
 * @retval 非0  失败
 */
IHAL_INT32 IHal_AO_GetMuteStatus(IHal_AudioHandle_t *handle, IHAL_INT32 *mute);

/**
 * @brief 设置放音音量
 * @param [in] handle : Audio handle
 * @param [in] value  : 音量值（0 - 100）
 * @retval 0    成功
 * @retval 非0 失败
 */
IHAL_INT32 IHal_AO_SetVolume(IHal_AudioHandle_t *handle, IHAL_INT32 value);

/**
 * @brief 获取放音音量
 * @param [in]  handle : Audio handle
 * @param [out] value  : 音量值（0 - 100）
 * @retval 0    成功
 * @retval 非0  失败
 */
IHAL_INT32 IHal_AO_GetVolume(IHal_AudioHandle_t *handle, IHAL_INT32 *value);

/**
 * @brief 设置放音Gain
 * @param [in] handle : Audio handle
 * @param [in] value  : 音量值
 * @retval 0    成功
 * @retval 非0 失败
 */
IHAL_INT32 IHal_AO_SetGain(IHal_AudioHandle_t *handle, IHAL_INT32 value);

/**
 * @brief 获取放音Gain
 * @param [in]  handle : Audio handle
 * @param [out] value  : 音量值
 * @retval 0    成功
 * @retval 非0  失败
 */
IHAL_INT32 IHal_AO_GetGain(IHal_AudioHandle_t *handle, IHAL_INT32 *value);

/************************** Audio Encoder *******************************/
/**
 * @brief 创建一个音频编码器通道
 * @param [in] attr : 音频编码器的属性
 * @retval IHal_AudioEnc_Handle_t 成功
 * @retval IHAL_RNULL             失败
 */
IHal_AudioEnc_Handle_t *IHal_AudioEnc_CreateChn(IHal_AudioEnc_Attr_t *attr);

/**
 * @brief 销毁一个音频编码器通道
 * @param [in] handle : AudioEnc handle
 * @retval 0    成功
 * @retval 非0  失败
 */
IHAL_INT32 IHal_AudioEnc_DestroyChn(IHal_AudioEnc_Handle_t *handle);

/**
 * @brief 放入一帧待编码的音频输入数据
 * @param [in] handle : AudioEnc handle
 * @param [in] frame  : 音频数据流信息
 * @param [in] block  : 等待类型（ #IMPP_NO_WAIT 或 #IMPP_WAIT_FOREVER ）
 * @retval 0    成功
 * @retval 非0  失败
 */
IHAL_INT32 IHal_AudioEnc_PutFrame(IHal_AudioEnc_Handle_t *handle, IHal_AudioStream_t *frame, IHAL_INT32 block);

/**
 * @brief 获取一帧编码后的音频输入数据
 * @param [in]  handle : AudioEnc handle
 * @param [out] stream : 音频数据流信息
 * @param [in]  block  : 等待类型（ #IMPP_NO_WAIT 或 #IMPP_WAIT_FOREVER ）
 * @retval 0    成功
 * @retval 非0  失败
 */
IHAL_INT32 IHal_AudioEnc_GetStream(IHal_AudioEnc_Handle_t *handle, IHal_AudioStream_t *stream, IHAL_INT32 block);

/**
 * @brief 释放一帧编码后的音频输入数据
 * @param [in] handle : AudioEnc handle
 * @param [in] stream : 音频数据流信息
 * @retval 0    成功
 * @retval 非0  失败
 */
IHAL_INT32 IHal_AudioEnc_ReleaseStream(IHal_AudioEnc_Handle_t *handle, IHal_AudioStream_t *stream);

/************************** Audio Decoder *******************************/
/**
 * @brief 创建一个音频解码器通道
 * @param [in] attr : 音频解码器的属性
 * @retval IHal_AudioDec_Handle_t 成功
 * @retval IHAL_RNULL             失败
 */
IHal_AudioDec_Handle_t *IHal_AudioDec_CreateChn(IHal_AudioDec_Attr_t *attr);

/**
 * @brief 销毁一个音频解码器通道
 * @param [in] handle : AudioDec handle
 * @retval 0    成功
 * @retval 非0  失败
 */
IHAL_INT32 IHal_AudioDec_DestroyChn(IHal_AudioDec_Handle_t *handle);

/**
 * @brief 放入一帧待解码的音频数据
 * @param [in] handle : AudioDec handle
 * @param [in] stream : 音频数据流信息
 * @param [in] block  : 等待类型（ #IMPP_NO_WAIT 或 #IMPP_WAIT_FOREVER ）
 * @retval 0    成功
 * @retval 非0  失败
 */
IHAL_INT32 IHal_AudioDec_PutStream(IHal_AudioDec_Handle_t *handle, IHal_AudioStream_t *stream, IHAL_INT32 block);

/**
 * @brief 获取一帧解码后的音频数据
 * @param [in]  handle : AudioEnc handle
 * @param [out] frame  : 音频数据流信息
 * @param [in]  block  : 等待类型（ #IMPP_NO_WAIT 或 #IMPP_WAIT_FOREVER ）
 * @retval 0    成功
 * @retval 非0  失败
 */
IHAL_INT32 IHal_AudioDec_GetFrame(IHal_AudioEnc_Handle_t *handle, IHal_AudioStream_t *frame, IHAL_INT32 block);

/**
 * @brief 释放一帧解码后的音频数据
 * @param [in] handle : AudioEnc handle
 * @param [in] frame  : 音频数据流信息
 * @retval 0    成功
 * @retval 非0  失败
 */
IHAL_INT32 IHal_AudioDec_ReleaseFrame(IHal_AudioEnc_Handle_t *handle, IHal_AudioStream_t *frame);

/************************** Audio Process *******************************/

/**
 * @brief 重采样初始化
 * @param [in] attr   : 重采样属性
 * @retval handle    成功
 * @retval IHAL_RNULL  失败
 */
IHal_Resampler_Handle_t *IHal_ResamlperInit(IHal_ResamplerAttr_t *attr);

/**
  * @brief 重采样处理
  * @param [in] handle   :handle
  * @param [in] indata   :源数据
  * @param [out] outdata :输出数据
  * @param [in]  insize  :源数据buffer大小
  * @retval 0    成功
  * @retval 非0  失败
  */
IHAL_INT32 IHal_ResamlperProcess(IHal_Resampler_Handle_t *handle, IHAL_INT16 *indata, IHAL_INT16 **outdata, IHAL_INT32 insize, IHAL_INT32 *outsize);

/**
  * @brief 设置重采样的采样率
  * @param [in] handle   :handle
  * @param [in] in_samplerate   :源数据采样率
  * @param [in] out_samplerate   :输出据采样率
  * @retval 0    成功
  * @retval 非0  失败
  */
IHAL_INT32 IHal_ResamlperSetRate(IHal_Resampler_Handle_t *handle, IHAL_UINT32 in_samplerate, IHAL_UINT32 out_samplerate);

/**
  * @brief 获取重采样的采样率
  * @param [in] handle   :handle
  * @param [out] in_samplerate   :源数据采样率
  * @param [out] out_samplerate   :输出据采样率
  * @retval 0    成功
  * @retval 非0  失败
  */
IHAL_INT32 IHal_ResamlperGetRate(IHal_Resampler_Handle_t *handle, IHAL_UINT32 *in_samplerate, IHAL_UINT32 *out_samplerate);

/**
  * @brief 设置重采样的质量参数
  * @param [in] handle   :handle
  * @param [in] target_quality : 质量[0-10]
  * @retval 0    成功
  * @retval 非0  失败
  */
IHAL_INT32 IHal_ResamlperSetQuality(IHal_Resampler_Handle_t *handle, IHAL_UINT32 target_quality);

/**
  * @brief 重采样反初始化
  * @param [in] handle   :handle
  * @retval 0   成功
  * @retval 非0  失败
  */
IHAL_INT32 IHal_ResamlperDeInit(IHal_Resampler_Handle_t *handle);

/**
 * @}
 */

/**
 * @}
 */

#endif  // __AUDIO_H__
