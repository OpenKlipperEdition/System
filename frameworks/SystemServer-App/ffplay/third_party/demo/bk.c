#include <stdio.h>
#include <unistd.h>
#include <assert.h>
#include <stdlib.h>
#include <stdbool.h>

#include "ffplay.h"

void printSelfMemoryStatus(const char *msg)
{
    printf("{%s}[%s](%d) : %s\n", __FILE__, __func__, __LINE__, msg); 

    FILE *statm = fopen("/proc/self/statm", "r");
    assert(statm);

    unsigned int pageSize = getpagesize();
    unsigned int size, resident, share;

    while(1){
        unsigned int tmpSize, tmpResident, tmpShare;

        fseek(statm, 0, SEEK_SET);
        fscanf(statm, "%u %u %u", &tmpSize, &tmpResident, &tmpShare);

        sleep(1);
        fseek(statm, 0, SEEK_SET);

        fscanf(statm, "%u %u %u", &size, &resident, &share);

        if(abs((int)size-(int)tmpSize) < 1000 && abs((int)resident-(int)tmpResident) < 100){
            break;
        } else {
            printf("{%s}[%s](%d) : %u %u -> %u %u\n", __FILE__, __func__, __LINE__, tmpSize, tmpResident, size, resident); 
        }
    }
    fclose(statm);

    unsigned int virtualMem = (unsigned int)(size * pageSize / 1024);
    unsigned int physicalMem= (unsigned int)(resident * pageSize / 1024);

    printf("{%s}[%s](%d) : virtualMem %u, physicalMem %u\n", __FILE__, __func__, __LINE__, virtualMem, physicalMem); 
}

int ret = 0;
int cnt = 500;

char *file = "/desktop.mp4";
int vol = 1;

void doTest000()
{
    while(1){
        ret = ffplay_init();
        assert(ret == 0);
        static bool first = true;
        if(first){
            first = false;
            printSelfMemoryStatus("init");
        }

        ret = ffplay_deinit();
        assert(ret == 0);

        if(cnt > 0){
            cnt--;
        } else {
            break;
        }
    }
    printSelfMemoryStatus("deinit");
}
void doTest000_1()
{
    while(1){
        ret = ffplay_init();
        assert(ret == 0);
        printSelfMemoryStatus("init");

        ret = ffplay_deinit();
        assert(ret == 0);
        printSelfMemoryStatus("deinit");
    }
}

void doTest001()
{
    ret = ffplay_init();
    assert(ret == 0);
    printSelfMemoryStatus("init");
    while(1){

        ret = ffplay_set_play_file(file);
        assert(ret == 0);

        if(cnt > 0){
            cnt--;
        } else {
            break;
        }
    }
    ret = ffplay_deinit();
    assert(ret == 0);
    printSelfMemoryStatus("deinit");
}
void doTest001_1()
{
    ret = ffplay_init();
    assert(ret == 0);
    printSelfMemoryStatus("init");
    while(1){
        ret = ffplay_set_play_file(file);
        assert(ret == 0);
        printSelfMemoryStatus("ffplay_set_play_file");
    }
    ret = ffplay_deinit();
    assert(ret == 0);
    printSelfMemoryStatus("deinit");
}

void doTest002()
{
    ret = ffplay_init();
    assert(ret == 0);
    printSelfMemoryStatus("init");

    ret = ffplay_set_play_file(file);
    assert(ret == 0);
    printSelfMemoryStatus("ffplay_set_play_file");

    ret = ffplay_set_video_fullsize();
    assert(ret == 0);
    ret = ffplay_set_audio_volume(vol);
    assert(ret == 0);
    ret = ffplay_set_play_loop(true);
    assert(ret == 0);
    ret = ffplay_set_autoclean(true);
    assert(ret == 0);
    ret = ffplay_set_layer_order(0);
    assert(ret == 0);
    printSelfMemoryStatus("ffplay_set_xx");

#if 1
    while(1){

        ret = ffplay_play_state_pause(true);
        assert(ret == 0);
        ret = ffplay_play_state_pause(false);
        assert(ret == 0);
        printSelfMemoryStatus("ffplay");

        if(cnt > 0){
            cnt--;
        } else {
            break;
        }
    }
#else
    ret = ffplay_play_state_pause(false);
    sleep(~0l);
#endif
    ret = ffplay_deinit();
    assert(ret == 0);
    printSelfMemoryStatus("deinit");
}
void doTest002_1()
{
    ret = ffplay_init();
    assert(ret == 0);
    printSelfMemoryStatus("init");

    ret = ffplay_set_play_file(file);
    assert(ret == 0);
    printSelfMemoryStatus("ffplay_set_play_file");

    ret = ffplay_set_video_fullsize();
    assert(ret == 0);
    ret = ffplay_set_audio_volume(vol);
    assert(ret == 0);
    ret = ffplay_set_play_loop(true);
    assert(ret == 0);
    ret = ffplay_set_autoclean(true);
    assert(ret == 0);
    ret = ffplay_set_layer_order(0);
    assert(ret == 0);
    printSelfMemoryStatus("ffplay_set_xx");

    while(1){
        ret = ffplay_play_state_pause(true);
        assert(ret == 0);
        ret = ffplay_play_state_pause(false);
        assert(ret == 0);
        printSelfMemoryStatus("ffplay");
        sleep(3);
    }
    ret = ffplay_deinit();
    assert(ret == 0);
    printSelfMemoryStatus("deinit");
}
void doTest002_2()
{
    ret = ffplay_init();
    assert(ret == 0);
    printSelfMemoryStatus("init");

    ret = ffplay_set_play_file(file);
    assert(ret == 0);
    printSelfMemoryStatus("ffplay_set_play_file");

    ret = ffplay_set_video_fullsize();
    assert(ret == 0);
    ret = ffplay_set_audio_volume(vol);
    assert(ret == 0);
    ret = ffplay_set_play_loop(true);
    assert(ret == 0);
    ret = ffplay_set_autoclean(true);
    assert(ret == 0);
    printSelfMemoryStatus("ffplay_set_xx");

    while(1){
        ret = ffplay_set_play_file(file);
        assert(ret == 0);
        ret = ffplay_set_layer_order(0);
        assert(ret == 0);
        printSelfMemoryStatus("ffplay_set_play_file");

        ret = ffplay_play_state_pause(false);
        assert(ret == 0);
        printSelfMemoryStatus("ffplay");

        sleep(3);
    }
    ret = ffplay_deinit();
    assert(ret == 0);
    printSelfMemoryStatus("deinit");
}

int main()
{
    printSelfMemoryStatus("begin");

    // mem leak, 2 hour 1500 KB
    /*doTest000();*/
    /*doTest000_1();*/

    // fix, no error
    // mem growth to a fixed value
    /*doTest001();*/
    /*doTest001_1();*/

    /*doTest002();*/
    /*doTest002_1();*/
    doTest002_2();

    return 0;
}
