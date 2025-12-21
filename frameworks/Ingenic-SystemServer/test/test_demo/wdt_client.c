#include <stdio.h>
#include <unistd.h>

#include "watchdog_client.h"

int main(int argc, char * argv[])
{
    wdt_clt_t *clt = NULL;
    int ret = 0;

    clt = alloc_wdt_clt("wdtClient");

    regiset_wdt_clt(clt);

    set_wdt_timeout(clt, 400);

    start_wdt(clt);

    while (1)
    {
        usleep(300000);
        feed_wdt(clt);
        //printf("feed dog\n");
    }

    stop_wdt(clt);

    unregiset_wdt_clt(clt);

    free_wdt_clt(clt);

    binder_threads_shutdown();
    return 0;
}

