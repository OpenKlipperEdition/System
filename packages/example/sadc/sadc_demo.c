#include <stdio.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#define ADC_MAGIC_NUMBER	'A'
#define ADC_ENABLE			_IO(ADC_MAGIC_NUMBER, 11)
#define ADC_DISABLE			_IO(ADC_MAGIC_NUMBER, 22)
#define ADC_SET_VREF		_IOW(ADC_MAGIC_NUMBER, 33, unsigned int)

int main(int argc, char *argv[])
{
	int fd;
	int data;
	char aux_name[30];
	unsigned int vref = 1800;
	sprintf(aux_name, "/dev/ingenic_adc_aux_%s", argv[1]);
	fd = open (aux_name,O_RDWR);
	if (fd < 0) {
		perror("open");
		return -1;
	}

	printf("ref = %d\n",vref);
	ioctl(fd,ADC_SET_VREF,&vref);
	while(1)
	{
		read (fd, (char *)&data, sizeof(data));
		printf("Voltage = %.3f\n", data * 1.0 / 1000);
		sleep(1);
	}

	close (fd);
	return 0;
}
