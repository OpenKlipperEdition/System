#include <stdio.h>
#include "ffplay.h"
#include <unistd.h>


char *a = "an";
#define ID "test"
int main(int argc, char **argv)
{
    int ret = 0;
	char *file = NULL;
	if(argc != 2){
		printf("请输入应用 + 文件名！\n");
		return 0;
	}
	file = argv[1];
    while(1)
    {

	ffplay_init(NULL,NULL);
        printf("\033[31m[%s]%s:%d %s\033[0m\n",__FILE__,__func__,__LINE__,file);
        ret = ffplay_set_play_file(file);
        if(ret) {
            return -1;
        }
        ffplay_set_autoclean(1);
        /* drm.setBrightness(30); */
        ffplay_set_audio_volume(1);
        ffplay_set_layer_order(0);
        ffplay_set_play_loop(1);
        ffplay_play_state_pause(0);
        printf("\033[31m[%s]%s:%d \033[0m\n",__FILE__,__func__,__LINE__);
        ffplay_set_video_fullsize();
        while(1)
            usleep(500*1000);
        //ffplay_play_state_pause(1);
        //usleep(500*1000);
        ffplay_play_state_stop();
    ffplay_deinit();
    usleep(500*1000);
    printf("\033[31m[%s]%s:%d \033[0m\n",__FILE__,__func__,__LINE__);
    }

    return 0;
}
