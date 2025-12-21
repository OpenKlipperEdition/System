/*
 * @Description: 
 * @Version: 
 * @Autor: 李石
 * @Date: 2021-11-20 20:49:02
 * @LastEditors: 李石
 * @LastEditTime: 2022-01-12 23:45:53
 */
#ifndef __RTSP_SERVER_H__
#define __RTSP_SERVER_H__
/*
 * a simple RTSP server demo
 * RTP over UDP/TCP H264/G711a 
*/

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_SESSION_NUM 64

#define RTSP_SUCCESS     0
#define RTSP_FAILURE     (-1)
#define RTSP_SUCC        RTSP_SUCCESS
#define RTSP_FAIL        RTSP_FAILURE
typedef enum _rtsp_codec_id{
    RTSP_CODEC_ID_NONE = 0,
    RTSP_CODEC_ID_VIDEO_H264 = 0x0001,    /*codec_data is SPS + PPS frames*/
    RTSP_CODEC_ID_VIDEO_H265,            /*codec_data is VPS + SPS + PPS frames*/
    RTSP_CODEC_ID_VIDEO_MPEG4,            /*now not support*/
    RTSP_CODEC_ID_AUDIO_G711A = 0x4001,    /*codec_data is NULL*/
    RTSP_CODEC_ID_AUDIO_G711U,            /*codec_data is NULL*/
    RTSP_CODEC_ID_AUDIO_G726,            /*codec_data is bitrate (int)*/
    RTSP_CODEC_ID_AUDIO_AAC,            /*codec_data is audio specific config (2bytes). frame type is ADTS*/
} rtsp_codec_id;

typedef struct _rtsp_cfg
{
    int session_count;
	char ipaddr[16];
	int  ip_len;
    struct {
        int port;
        char suffix[200];
        rtsp_codec_id videoCodec;
        rtsp_codec_id audioCodec;
    } session_cfg[MAX_SESSION_NUM];
}rtsp_cfg;

typedef struct _rtsp_sdp
{
    int session_count;
    struct {
        char url[240];
    } session_sdp[MAX_SESSION_NUM];
}rtsp_sdp;
#define RTSP_CALLBACK 
typedef void (RTSP_CALLBACK* rtspClientConnect)(char *clientIP,int clientPort);
typedef void (RTSP_CALLBACK* rtspClientDisconnect)(char *clientIP);
int rtsp_init(rtsp_cfg *cfg,rtsp_sdp *sdp,rtspClientConnect onConnect,rtspClientDisconnect onDisconnect);
int rtsp_server_write_video(int chn, const uint8_t *frame, int len, uint64_t ts);
int rtsp_server_write_audio(int chn, const uint8_t *frame, int len, uint64_t ts);
int rtsp_uinit();
void rtsp_codec_to_char(rtsp_codec_id codecID,char *codecBuf,int buflen);

#ifdef __cplusplus
}
#endif
#endif
