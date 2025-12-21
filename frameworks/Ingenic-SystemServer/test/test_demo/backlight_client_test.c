#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "backlight_client.h"


int main(int argc, char * argv[]){

	backlight_handle_t* hdl = NULL;
	int level = 0;
	hdl = backlight_server_request("test_client");
	for(int i = 0; i <= 10; i++){
		if(i == 10)
			backlight_set_level(hdl,i * 10 - 1);
		else if(i == 0)
			backlight_set_level(hdl,1);
		else
			backlight_set_level(hdl,i * 10);

		backlight_get_level(hdl,&level);
		sleep(1);
	}
	printf("test backlight_on_off \r\n");
	backlight_on_off(hdl,0);
	sleep(1);
	backlight_on_off(hdl,1);
	sleep(1);
	backlight_server_release(hdl);
    binder_threads_shutdown();
    return 0;
}

