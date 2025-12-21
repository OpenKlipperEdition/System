# SYSTEM SERVER NetWork API使用说明
##  int32_t NetWorkManager_Init(NET_APP_LEVEL_t level)
### 功能说明
初始化网络管理模块
### 参数
level: 网络应用等级(0-1)
### 参数说明
LEVEL_0: 0级应用，核心系统应用，只能注册一个；可控制网络服务核心组件的启动和停止以及网络的其它功能
LEVEL_1: 1级应用，系统应用，支持网络配置连接和断开连接等
### 返回值
成功：0\
失败: 非0

## int32_t NetWorkManager_DeInit(void)
### 功能说明
关闭网络管理模块，资源释放
### 参数 
无
### 返回值
成功：0\
失败: 非0

## int32_t WifiEnable(void)
### 功能说明
使能Wifi功能 （0级应用有效）
### 参数
无
### 返回值
成功：0\
失败: 非0

## int32_t WifiDisable(void)
### 功能说明
关闭Wifi功能 （0级应用有效）
### 参数
无
### 返回值
成功：0\
失败: 非0

## int32_t WifiScan(void)
### 功能说明
Wifi开始扫描
### 参数
无
### 返回值
成功：0\
失败: 非0

## int32_t WifiGetScanResults(WifiScanResult_t** results,int32_t wait_ms)
### 功能说明
获取Wifi扫描结果
### 参数
results: 用于存放扫描结果 (使用结束后需要手动释放)
wait_ms: 等待扫描结果的时间，单位 ms
### 返回值
成功：0\
失败: 非0

## int32_t WifiAddConfig(WifiConfigInfo_t* config)
### 功能说明
添加wifi网络配置
### 参数
config: 网络配置参数结构体，用于填充热点信息
### 返回值
成功: nid (配置序号)
失败: -1

## int32_t WifiDeleteConfig(int32_t nid)
### 功能说明
删除网络配置信息
### 参数
nid: 要删除的网络配置的nid
### 返回值
成功：0\
失败: 非0

## int32_t WifiSaveCurrentConfig(void)
### 功能说明
保存当前网络配置
### 参数
无
### 返回值
成功：0\
失败: 非0

## int32_t WifiEnableNetwork(int32_t nid)
### 功能说明
使能网络配置
### 参数
nid: 网络配置的nid
### 返回值
成功：0\
失败: 非0

## int32_t WifiDisableNetwork(int32_t nid)
### 功能说明
禁用网络配置
### 参数
nid: 网络配置的nid
### 返回值
成功：0\
失败: 非0

## int32_t WifiConnectNetwork(int32_t nid,int16_t wait_s)
### 功能说明
使用指定网络配置连接
### 参数
nid: 网络配置的nid
wait_s: 等待连接成功的时间，单位s
### 返回值
成功：0\
失败: 非0

## int32_t WifiDisconnectNetwork(int32_t nid)
### 功能说明
断开网络连接
### 参数
nid: 网络配置的nid
### 返回值
成功：0\
失败: 非0

## int32_t WifiGetLinkedSta(wlan_connect_sta_t* sta)
### 功能说明
获取设备的网络连接状态
### 参数
sta: 存放网络连接状态
### 返回值
成功：0\
失败: 非0

## int32_t WifiSetListenCallback(NetState_Cb_t* cb)   
### 功能说明
设置网络事件订阅的回调函数
### 参数
cb: 网络回调函数结构
### 返回值
成功：0\
失败: 非0