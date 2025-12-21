# SystemServer CameraAPI使用说明

## ISS_CamHandle_t ISS_CameraInit(CameraInitParam_t *param)
### 参数
param: 用于描述需要打开的节点、分辨率等信息
### 返回值
成功: 返回一个ISS_CamHandle_t句柄 
失败: 返回 NULL

## int32_t ISS_CameraDeInit(ISS_CamHandle_t handle)
### 参数
handle: 需要反初始化的句柄
### 返回值
成功: 0
失败: 非0

## int32_t ISS_GetCameraData(ISS_CamHandle_t handle,DataBuffer_t* data)
### 参数
handle: 初始化返回的Camera句柄
data : 用于返回Data的信息(地址、大小等)
### 返回值
成功: 0
失败: 非0

## int32_t ISS_ReleaseCameraData(ISS_CamHandle_t handle,DataBuffer_t* data) 
### 参数
handle: 初始化返回的Camera句柄
data : 需要释放的data信息
### 返回值
成功: 0
失败: 非0


# 测试Demo说明
## demo路径
test/test_demo/camera_test.c
## 测试说明
该测试初始化了３个设备节点，并为每个节点创建了处理数据的线程(将数据写入文件)