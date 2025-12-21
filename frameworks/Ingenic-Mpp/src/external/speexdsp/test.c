#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <assert.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>

#include "speex_resampler.h"

int get_num_samples_10ms(unsigned int sampele_rate)
{
	switch(sampele_rate){
		case 8000:
			return 80;
		case 16000:
			return 160;
		case 24000:
			return 240;
		case 32000:
			return 320;
		case 44100:
			return 441;
		default:
			return -1;
	}
}

int main(int argc,char** argv)
{
	if(argc < 6){
		printf("param failed ./resampler in.pcm out.pcm in_samplerate out_samplerate chans\r\n");
		return -1;
	}
	char* inputfile_name = argv[1];
	char* outputfile_name = argv[2];
	int input_samplerate = atoi(argv[3]);
	int output_samplerate = atoi(argv[4]);
	int channels = atoi(argv[5]);
	printf("input sampele_rate = %d  out_samplerate = %d channels = %d \r\n",input_samplerate,output_samplerate,channels);
	int numsamples_in = get_num_samples_10ms(input_samplerate);
	int numsamples_out = get_num_samples_10ms(output_samplerate);
	if(numsamples_in < 0){
		printf("input sampele_rate failed");
		return -1;
	}
	if(numsamples_out < 0){
		printf("out sampele_rate failed");
		return -1;
	}
	int ifd = open(inputfile_name,O_RDONLY);
	if(ifd < 0){
		printf("open %s failed \r\n",inputfile_name);
		return -1;
	}

	int ofd = open(outputfile_name,O_RDWR | O_CREAT | O_TRUNC,0666);
	if(ofd < 0){
		printf("open %s failed \r\n",outputfile_name);
		return -1;
	}

	short* srcbuffer = (short*)malloc(numsamples_in * sizeof(short) * channels);
	short* outbuffer = (short*)malloc(numsamples_out * sizeof(short) * channels);
	int err;
	SpeexResamplerState * state = speex_resampler_init(channels,input_samplerate,output_samplerate,6,&err);
	if(state == NULL){
		printf("resampler init failed\r\n");
		return -1;
	}
	speex_resampler_set_rate(state,input_samplerate,output_samplerate);
	int outlen = numsamples_out;
	int ret = read(ifd,srcbuffer,numsamples_in * sizeof(short) * channels);
	int inlen = numsamples_in;
	int i = 0;
	int j = 0;
	while(ret == numsamples_in * sizeof(short) * channels){
		outlen = numsamples_out;
		inlen = numsamples_in;
		if(channels == 1){
			ret = speex_resampler_process_int(state,0,(const short*)srcbuffer,&inlen,
					outbuffer,&outlen);
		} else {
			/* for(j = 0; j < numsamples_in - 2; j += 2){ */
			/* 	inbuffer0[i++] = (short)srcbuffer[j]; */
			/* } */
			/* ret = speex_resampler_process_int(state,0,(const short*)inbuffer0,&inlen, */
			/* 		outbuffer0,&outlen); */
			/* for(j = 0,i = 0; j < outlen; j += 2){ */
			/* 	outbuffer[j] = outbuffer0[i++]; */
			/* } */
			/* i = 0; */
			/* outlen = numsamples_out; */
			/* inlen = numsamples_in; */
			/* for(j = 1,i = 0; j < numsamples_out; j += 2){ */
			/* 	inbuffer0[i++] = srcbuffer[j]; */
			/* } */
			/* ret = speex_resampler_process_int(state,1,(const short*)inbuffer0,&inlen, */
			/* 		outbuffer0,&outlen); */
			/* for(j = 1,i = 0; j < numsamples_out; j += 2){ */
			/* 	outbuffer[j] = outbuffer0[i++]; */
			/* } */

			/* ret = speex_resampler_process_int(state,0,(const short*)inbuffer0,&inlen, */
			/* 		outbuffer0,&outlen); */
			/* ret = speex_resampler_process_int(state,1,(const short*)inbuffer1,&inlen, */
			/* 		outbuffer1,&outlen); */
			/* for(int j = 0; j < numsamples_out; j+2){ */
			/* 	outbuffer[j] = */
			/* } */
			ret = speex_resampler_process_interleaved_int(
				state,(const short*)srcbuffer,
				&inlen,outbuffer,&outlen);
		}
		if(ret){
			printf("process failed \r\n");
		}
		printf("process ok ..... ret = %d outlen = %d \r\n",ret,outlen);
		write(ofd,outbuffer,outlen * sizeof(short) * channels);
		ret = read(ifd,srcbuffer,numsamples_in  * sizeof(short) * channels);
	}
	close(ofd);
	close(ifd);
	free(srcbuffer);
	free(outbuffer);
	speex_resampler_destroy(state);
	return 0;
}


