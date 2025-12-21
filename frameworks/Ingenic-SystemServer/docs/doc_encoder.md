# SYSTEM SERVER Encoder API使用说明

## API使用说明
### EncChnHandle_t ISS_CreateEncodeChn(EncoderChnParam_t* param) 
#### 说明: 创建一个编码通道
#### 参数：
param: 编码通道参数
#### 返回值:
成功: 编码通道的Handle \
失败: NULL

### int ISS_DestroyEncodeChn(EncChnHandle_t handle)
#### 说明:销毁一个编码通道
#### 参数:
handle: 创建通道时返回的句柄Handle
#### 返回值:
成功: 0 \
失败: 非0                                               
                                                                                             
### int ISS_EncodeChn_SetFinishCallBack(EncChnHandle_t handle,EncoderFinish_Cb_t* cb)
#### 说明: 设置编码完成时的回调函数
#### 参数:
handle: 编码通道的句柄 \
cb: 回调函数
#### 返回值:
成功: 0 \
失败: 非0 
                                                                                               
### int ISS_EncodeChn_RequestSrcBuffer(EncChnHandle_t handle,DataBuffer_t* buffer)
#### 说明: 获取一个源数据buffer (仅适用于非Camera源数据类型的编码通道)           
#### 参数: 
handle: 编码通道句柄 Handle \
buffer: 用于保存获取到的buffer信息
#### 返回值:
成功: 0 \
失败: 非0 
                                                                                               
### int ISS_EncodeChn_SendSrcFrame(EncChnHandle_t handle,DataBuffer_t* frame) 
#### 说明: 向编码通道送入一帧原始数据 (仅适用于非Camera源数据类型的编码通道)           
#### 参数: 
handle: 编码通道句柄 Handle \
buffer: 原始数据的buffer信息
#### 返回值:
成功: 0 \
失败: 非0                    
                                                                                               
### int ISS_EncodeChn_Start(EncChnHandle_t handle)
#### 说明: 编码通道开始工作   
#### 参数: 
handle: 编码通道句柄 Handle 
#### 返回值:
成功: 0 \
失败: 非0
                                                                                               
### int ISS_EncodeChn_Stop(EncChnHandle_t handle)
#### 说明: 编码通道停止工作   
#### 参数: 
handle: 编码通道句柄 Handle 
#### 返回值:
成功: 0 \
失败: 非0    

### int ISS_EncodeChn_IDR_Request(EncChnHandle_t handle)
#### 说明: 请求一个IDR帧，发送该请求后，会立即在后续的２到3帧返回一个IDR帧   
#### 参数: 
handle: 编码通道句柄 Handle 
#### 返回值:
成功: 0 \
失败: 非0                                          
                                                                                               
### int ISS_EncodeChn_SetQPBounds(EncChnHandle_t handle,int32_t maxQp,int32_t minQp)            
#### 说明: 重新设置编码通道的Qp值  
#### 参数: 
maxQp: 最大QP值
minQp: 最小QP值
#### 返回值:
成功: 0 \
失败: 非０

### int ISS_EncodeChn_SetBitrate(EncChnHandle_t handle,uint32_t max_bitrate,uint32_t min_bitrate)
#### 说明: 重新设置编码通道的码率值
#### 参数: 
max_bitrate: 最大编码码率
min_bitrate: 最小编码码率
#### 返回值:
成功: 0 \
失败: 非０
