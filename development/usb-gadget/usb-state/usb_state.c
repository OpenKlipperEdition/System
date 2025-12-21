#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <dirent.h>
#include <stdbool.h>
#include <time.h>
#include <stdarg.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <sys/ioctl.h>
#include <sys/types.h>

static const char *prg_name;


static void usage(int status)
{
    fprintf(stderr, "%s usage\n", prg_name);
    fprintf(stderr, "usb state list:\n");
    fprintf(stderr, "\t not attached\n");
    fprintf(stderr, "\t attached\n");
    fprintf(stderr, "\t powered\n");
    fprintf(stderr, "\t reconnecting\n");
    fprintf(stderr, "\t unauthenticated\n");
    fprintf(stderr, "\t default\n");
    fprintf(stderr, "\t addresssed\n");
    fprintf(stderr, "\t configured\n");
    fprintf(stderr, "\t suspended\n");

    fprintf(stderr, "   Usage: %s <udc_path> [state] \n", prg_name);
    fprintf(stderr, "   %s /sys/class/udc/13500000.otg \"suspended\"\n", prg_name);
    exit(status);
}


int main(int argc, char *argv[])
{
    int i;
    int fd;
    int fd1;
    fd_set efds;
    int ret, len;
    char buf[128];
    const char *udc_path = NULL;

    char state_buf[128];

    prg_name = argv[0];

    if (argc < 2)
        usage(-1);

    udc_path = argv[1];

    memset(state_buf, 0, sizeof(state_buf));
    sprintf(state_buf, "%s/state", udc_path);
    fd = open(state_buf, O_RDONLY);
    if (fd < 0) {
        printf("open %s fail\n", state_buf);
        return -1;
    }

    memset(state_buf, 0, sizeof(state_buf));
    sprintf(state_buf, "%s/power_type", udc_path);
    fd1 = open(state_buf, O_RDONLY);
    if (fd1 < 0) {
        printf("open %s fail\n", state_buf);
    }

    while (1) {
        FD_ZERO(&efds);
        FD_SET(fd, &efds);

        ret = select(fd + 1, NULL, NULL, &efds, NULL);
        if (ret == -1) {
            fprintf(stderr, "select error\n");
            break;
        }

        if (FD_ISSET(fd, &efds)) {
            if (fd1 >= 0) {
                memset(buf, 0, sizeof(buf));
                len = read(fd1, buf, sizeof(buf));
                lseek(fd1, 0, SEEK_SET);
                printf("usb power : %s\n", buf);
            }

            memset(buf, 0, sizeof(buf));
            len = read(fd, buf, sizeof(buf));
            lseek(fd, 0, SEEK_SET);
            printf("usb state : %s\n", buf);

            for (i = 2; i < argc; i++) {
                ret = strncmp(buf, argv[i], len - 1);
                if (ret == 0)
                    goto go_out;
            }
        }
    }
go_out:

    close(fd);

    if (fd1 >= 0)
        close(fd1);

    return 0;
}