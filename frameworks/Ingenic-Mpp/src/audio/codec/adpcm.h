/*
** adpcm.h - include file for adpcm coder.
**
** Version 1.0, 7-Jul-92.
*/

#ifndef _ADPCM_H_
#define _ADPCM_H_

typedef struct adpcm_state_t {
        short       valprev;        /* Previous output value */
        char        index;          /* Index into stepsize table */
} adpcm_state;


int adpcm_encode(char *outdata, short *indata, int len);
int adpcm_decode(short *outdata, char *indata, int len);


#endif

