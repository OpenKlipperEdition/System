#include <stdio.h>
#include "ffplay.h"
#include <unistd.h>

#include <mcheck.h>

static int cnt =0;
int max = 100;

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
    
    char cmd[1024];
    int pid = getpid();
    sprintf(cmd, "cat /proc/%d/maps > /m0.txt", pid);
    system(cmd);

    /*mtrace();*/
    while(1) {   
	    ffplay_init();
        ret = ffplay_set_play_file(file);
        if(ret) {
            return -1;
        }
        ffplay_set_video_fullsize();
        ffplay_set_audio_volume(20); // 0 ~ 99
        ffplay_set_layer_order(0);
        ffplay_play_state_pause(0);
//        ffplay_set_video_fullsize();
        sleep(5);
        //usleep(200*1000);

        ffplay_play_state_stop();
        ffplay_deinit();

cnt++;

        //break;
        //     getchar();
        printf("<%s>[%s %d]: cnt %d/%d\n", __FILE__, __func__, __LINE__, cnt, max);
        /*if (cnt > max)*/
        /*{*/
            /*break;*/
        /*}*/
    }
    /*muntrace();*/
    return 0;
}
