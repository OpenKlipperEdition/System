#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#define UBI_DWC2_MODE_P "/sys/devices/platform/ahb2/13500000.otg/dwc2_mode"
#define UBI_UDC_P "/sys/kernel/config/usb_gadget/demo/UDC"
#define UBI_MSG   "13500000.otg\n"

int open_and_check(const char *path, unsigned int flags)
{
  int fd = 0;
  fd = open(path, flags);
  if (fd <= 0) {
    printf("open %s: %s\n", path, strerror(errno));
    exit(-1);
  }
}

int main(int argc, char **argv)
{
  int mode_fp, udc_fp;
  char line[64];
  int ret;

  mode_fp = open_and_check(UBI_DWC2_MODE_P, O_RDONLY);

  ret = read(mode_fp, line, sizeof(line));
  if (ret <= 0) {
    printf("read %s: %s\n", UBI_DWC2_MODE_P, strerror(errno));
    return -1;
  }

  /* Timely release of useless resources */
  close(mode_fp);

  if (strcmp(line, "host") != 0) {
    while(1) {
      udc_fp = open_and_check(UBI_UDC_P, O_RDONLY);
      ret = read(udc_fp, line, sizeof(line));
      close(udc_fp);

      /* empty file get is '\n' */
      if (ret == 1){
	udc_fp = open_and_check(UBI_UDC_P, O_WRONLY);
	int n = write(udc_fp, UBI_MSG, strlen(UBI_MSG));
	if (n <= 0 ) {
	  printf("write %s: %s\n", UBI_UDC_P, strerror(errno));
	}
	close(udc_fp);
      }
      usleep(1200000);
    }
  }
}
