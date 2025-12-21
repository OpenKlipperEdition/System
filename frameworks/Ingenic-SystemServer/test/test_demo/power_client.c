#include "binder_common.h"
#include "binder_ipc.h"
#include "binder_io.h"
#include <sys/time.h>

#include "power_client.h"

int main(int argc, char * argv[])
{

    int ret = 0;
    power_clt_t *clt = NULL;

    clt  = alloc_power_clt("power_test");


    ret = power_wakeup(clt, 10);
    if (ret != 0)
        printf("power_wakeup fail\n");
    ret = power_suspend(clt, 0);
    if (ret != 0)
        printf("power_suspend fail\n");
    
    //power_suspend(clt, 10);
    //printf("the system will suspend in 10 second\n");

    //power_down(clt, 10);
    //printf("the system will power_down in 10 second\n");

    free_power_clt(clt);
    binder_threads_shutdown();
    return 0;
}

