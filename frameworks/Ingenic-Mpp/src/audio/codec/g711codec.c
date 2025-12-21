#include "g711codec.h"
static short seg_aend[8] = {0x1F, 0x3F, 0x7F, 0xFF,
                            0x1FF, 0x3FF, 0x7FF, 0xFFF
                           };

static short seg_uend[8] = {0x3F, 0x7F, 0xfF, 0x1FF,
                            0x3FF, 0x7FF, 0xFFF, 0x1FFF
                           };

static int search(int val, short    *table, int size)
{
        int i;

        for (i = 0; i < size; i++) {
                if (val <= *table++) {
                        return (i);
                }
        }
        return (size);
}

/*
* alaw2linear() - Convert an A-law value to 16-bit linear PCM
*
*/
static int alaw2linear(unsigned char a_val)
{
        int t;
        int seg;

        a_val ^= 0x55;

        t = (a_val & QUANT_MASK) << 4;
        seg = ((unsigned)a_val & SEG_MASK) >> SEG_SHIFT;
        switch (seg) {
        case 0:
                t += 8;
                break;
        case 1:
                t += 0x108;
                break;
        default:
                t += 0x108;
                t <<= seg - 1;
        }
        return ((a_val & SIGN_BIT) ? t : -t);
}


/*
* ulaw2linear() - Convert a u-law value to 16-bit linear PCM
*
* First, a biased linear code is derived from the code word. An unbiased
* output can then be obtained by subtracting 33 from the biased code.
*
* Note that this function expects to be passed the complement of the
* original code word. This is in keeping with ISDN conventions.
*/
static int ulaw2linear(unsigned char u_val)
{
        int t;

        /* Complement to obtain normal u-law value. */
        u_val = ~u_val;

        /*
        * Extract and bias the quantization bits. Then
        * shift up by the segment number and subtract out the bias.
        */
        t = ((u_val & QUANT_MASK) << 3) + BIAS;
        t <<= ((unsigned)u_val & SEG_MASK) >> SEG_SHIFT;

        return ((u_val & SIGN_BIT) ? (BIAS - t) : (t - BIAS));
}


/*
 * linear2alaw() - Convert a 16-bit linear PCM value to 8-bit A-law
 *
 */
unsigned char linear2alaw(int pcm_val)  /* 2's complement (16-bit range) */
{
        int     mask;
        int     seg;
        unsigned char   aval;

        pcm_val = pcm_val >> 3;

        if (pcm_val >= 0) {
                mask = 0xD5;        /* sign (7th) bit = 1 */
        } else {
                mask = 0x55;        /* sign bit = 0 */
                pcm_val = -pcm_val - 1;
        }

        /* Convert the scaled magnitude to segment number. */
        seg = search(pcm_val, seg_aend, 8);

        /* Combine the sign, segment, and quantization bits. */

        if (seg >= 8) {     /* out of range, return maximum value. */
                return (0x7F ^ mask);
        } else {
                aval = seg << SEG_SHIFT;
                if (seg < 2) {
                        aval |= (pcm_val >> 1) & QUANT_MASK;
                } else {
                        aval |= (pcm_val >> seg) & QUANT_MASK;
                }
                return (aval ^ mask);
        }
}


/*
 * linear2ulaw() - Convert a linear PCM value to u-law
 *
 */
unsigned char linear2ulaw(int pcm_val)  /* 2's complement (16-bit range) */
{
        int     mask;
        int     seg;
        unsigned char   uval;

        pcm_val = pcm_val >> 2;

        if (pcm_val < 0) {
                pcm_val = -pcm_val;
                mask = 0x7F;
        } else {
                mask = 0xFF;
        }

        if (pcm_val > CLIP) {
                pcm_val = CLIP;
        }

        pcm_val += (BIAS >> 2);

        seg = search(pcm_val, seg_uend, 8);
        if (seg >= 8) {     /* out of range, return maximum value. */
                return (unsigned char)(0x7F ^ mask);
        } else {
                uval = (seg << 4) | ((pcm_val >> (seg + 1)) & 0xF);
                return (uval ^ mask);
        }
}


int g711a_decode(short amp[], const unsigned char g711a_data[], int g711a_bytes)
{
        int i;
        int samples;
        unsigned char code;
        int sl;

        for (samples = i = 0; ;) {
                if (i >= g711a_bytes) {
                        break;
                }
                code = g711a_data[i++];

                sl = alaw2linear(code);

                amp[samples++] = (short) sl;
        }
        return samples;
}

int g711u_decode(short amp[], const unsigned char g711u_data[], int g711u_bytes)
{
        int i;
        int samples;
        unsigned char code;
        int sl;

        for (samples = i = 0;;) {
                if (i >= g711u_bytes) {
                        break;
                }
                code = g711u_data[i++];

                sl = ulaw2linear(code);

                amp[samples++] = (short) sl;
        }
        return samples;
}

int g711a_encode(unsigned char g711_data[], const short amp[], int len)
{
        int i;

        for (i = 0;  i < len;  i++) {
                //  printf("data[%d] = %d\n", i, amp[i]);
                g711_data[i] = linear2alaw(amp[i]);
        }

        return len;
}

int g711u_encode(unsigned char g711_data[], const short amp[], int len)
{
        int i;

        for (i = 0;  i < len;  i++) {
                g711_data[i] = linear2ulaw(amp[i]);
        }

        return len;
}

