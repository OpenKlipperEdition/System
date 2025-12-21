#include <msa.h>
#include "convert.h"


#define MIN(a, b) (a > b ? b : a)

#define LOAD_MAC(v,p,i) v[i] = __builtin_msa_ld_b(p,i * 16);
#define STORE_MAC(v,p,i) __builtin_msa_st_b(v[i],p + i * nv12->stride,0)


static void _tile420_nv12_y_msa(struct tile420_fmt tile ,struct nv12_fmt* nv12)
{
    int width = MIN(nv12->width,tile.width);
    int height = MIN(nv12->height,tile.height);
    int aligned_height = height / 16 * 16;
    int residue_height = height - aligned_height;
    int aligned_width = width / 16 * 16;
    int residue_width = width - aligned_width;
    uint8_t *dst = nv12->d;
    uint8_t *src_y = tile.y;
    int y = 0;
    for(;y < aligned_height;y += 16) {
        uint8_t *end = dst + aligned_width;
        while(dst < end) {
            v16u8 v_src_y[16];

            LOAD_MAC(v_src_y,src_y,0);  LOAD_MAC(v_src_y,src_y,1);  LOAD_MAC(v_src_y,src_y,2);  LOAD_MAC(v_src_y,src_y,3);
            LOAD_MAC(v_src_y,src_y,4);  LOAD_MAC(v_src_y,src_y,5);  LOAD_MAC(v_src_y,src_y,6);  LOAD_MAC(v_src_y,src_y,7);
            LOAD_MAC(v_src_y,src_y,8);  LOAD_MAC(v_src_y,src_y,9);  LOAD_MAC(v_src_y,src_y,10); LOAD_MAC(v_src_y,src_y,11);
            LOAD_MAC(v_src_y,src_y,12); LOAD_MAC(v_src_y,src_y,13); LOAD_MAC(v_src_y,src_y,14); LOAD_MAC(v_src_y,src_y,15);

            STORE_MAC(v_src_y,dst, 0);  STORE_MAC(v_src_y,dst, 1);  STORE_MAC(v_src_y,dst, 2);  STORE_MAC(v_src_y,dst, 3);
            STORE_MAC(v_src_y,dst, 4);  STORE_MAC(v_src_y,dst, 5);  STORE_MAC(v_src_y,dst, 6);  STORE_MAC(v_src_y,dst, 7);
            STORE_MAC(v_src_y,dst, 8);  STORE_MAC(v_src_y,dst, 9);  STORE_MAC(v_src_y,dst, 10); STORE_MAC(v_src_y,dst, 11);
            STORE_MAC(v_src_y,dst,12);  STORE_MAC(v_src_y,dst,13);  STORE_MAC(v_src_y,dst, 14); STORE_MAC(v_src_y,dst, 15);
            src_y += 256;
            dst += 16;
        }

        if(residue_width) {
            for(int j = 0;j < 16;j++) {
                for(int i = 0;i < residue_width;i++) {
                    dst[j * nv12->stride + i] = src_y[i];
                }
                src_y += residue_width;
            }
        }

        dst = nv12->d + (y + 16) * nv12->stride;
        src_y = tile.y + (y + 16) * tile.stride;
    }

    if(residue_height) {
        v16u8 v_src_y[1];
        uint8_t *end = dst + aligned_width;
        while(dst < end) {
            for(int j = 0;j < residue_height;j++) {
                LOAD_MAC(v_src_y,src_y,0);
                STORE_MAC(v_src_y,dst + j * nv12->stride, 0);
                src_y += 16;
            }
            dst += 16;
        }

        if(residue_width) {
            for(int j = 0;j < residue_height;j++) {
                for(int i = 0;i < residue_width;i++) {
                    dst[j * nv12->stride + i] = src_y[i];
                }
                src_y += residue_width;
            }
        }
    }
}

static void _tile420_nv12_uv_msa(struct tile420_fmt tile ,struct nv12_fmt* nv12)
{
    uint8_t *p_uv = nv12->d + nv12->height * nv12->stride;
    uint8_t *dst = p_uv;
    uint8_t *src_uv = tile.uv;

    int width = MIN(nv12->width,tile.width);
    int height = MIN(nv12->height / 2,tile.height / 2);

    int aligned_height = height / 8 * 8;
    int residue_height = height - aligned_height;
    int aligned_width = width / 16 * 16;
    int residue_width = width - aligned_width;

    int y = 0;
    for(;y < aligned_height;y += 8) {
        uint8_t *end = dst + aligned_width;
        while(dst < end){
            v16u8 v_uv[8];
            v16u8 v_v[4];
            v16u8 v_u[4];

            LOAD_MAC(v_uv,src_uv,0);  LOAD_MAC(v_uv,src_uv,1);  LOAD_MAC(v_uv,src_uv,2);  LOAD_MAC(v_uv,src_uv,3);
            LOAD_MAC(v_uv,src_uv,4);  LOAD_MAC(v_uv,src_uv,5);  LOAD_MAC(v_uv,src_uv,6);  LOAD_MAC(v_uv,src_uv,7);
            v_u[0] =  __builtin_msa_ilvr_d(v_uv[1],v_uv[0]);
            v_v[0] =  __builtin_msa_ilvl_d(v_uv[1],v_uv[0]);

            v_u[1] =  __builtin_msa_ilvr_d(v_uv[3],v_uv[2]);
            v_v[1] =  __builtin_msa_ilvl_d(v_uv[3],v_uv[2]);

            v_u[2] =  __builtin_msa_ilvr_d(v_uv[5],v_uv[4]);
            v_v[2] =  __builtin_msa_ilvl_d(v_uv[5],v_uv[4]);

            v_u[3] =  __builtin_msa_ilvr_d(v_uv[7],v_uv[6]);
            v_v[3] =  __builtin_msa_ilvl_d(v_uv[7],v_uv[6]);

            v_uv[0] = __builtin_msa_ilvr_b(v_v[0],v_u[0]);
            v_uv[1] = __builtin_msa_ilvl_b(v_v[0],v_u[0]);

            v_uv[2] = __builtin_msa_ilvr_b(v_v[1],v_u[1]);
            v_uv[3] = __builtin_msa_ilvl_b(v_v[1],v_u[1]);

            v_uv[4] = __builtin_msa_ilvr_b(v_v[2],v_u[2]);
            v_uv[5] = __builtin_msa_ilvl_b(v_v[2],v_u[2]);

            v_uv[6] = __builtin_msa_ilvr_b(v_v[3],v_u[3]);
            v_uv[7] = __builtin_msa_ilvl_b(v_v[3],v_u[3]);

            STORE_MAC(v_uv,dst, 0);  STORE_MAC(v_uv,dst, 1);  STORE_MAC(v_uv,dst, 2);  STORE_MAC(v_uv,dst, 3);
            STORE_MAC(v_uv,dst, 4);  STORE_MAC(v_uv,dst, 5);  STORE_MAC(v_uv,dst, 6);  STORE_MAC(v_uv,dst, 7);


            src_uv += 128;
            dst += 16;
        }

        if(residue_width) {
            for(int j = 0;j < 8;j++) {
                for(int i = 0;i < residue_width / 2;i++) {
                    dst[j * nv12->stride + 2 * i] = src_uv[i];
                    dst[j * nv12->stride + 2 * i + 1] = src_uv[residue_width / 2 + i];
                }
                src_uv += residue_width;
            }
        }
        dst = p_uv + (y + 8) * nv12->stride;
        src_uv = tile.uv + (y + 8) * tile.stride;
    }

    if(residue_height) {
        uint8_t *end = dst + aligned_width;
        while(dst < end) {
            for(int j = 0;j < residue_height;j++) {
                v16u8 v_uv;
                v16u8 v_v;
                v16u8 v_u;
                v_uv = __builtin_msa_ld_b(src_uv,0);
                v_u =  __builtin_msa_ilvr_d(v_uv,v_uv);
                v_v =  __builtin_msa_ilvl_d(v_uv,v_uv);
                v_uv = __builtin_msa_ilvr_b(v_v,v_u);
                __builtin_msa_st_b(v_uv,dst + j * nv12->stride,0);
                src_uv += 16;
            }
            dst += 16;
        }

        if(residue_width) {
            for(int j = 0;j < residue_height;j++) {
                for(int i = 0;i < residue_width / 2;i++) {
                    dst[j * nv12->stride + 2 * i] = src_uv[i];
                    dst[j * nv12->stride + 2 * i + 1] = src_uv[residue_width / 2 + i];
                }
                src_uv += residue_width;
            }
        }
    }
}

IHAL_INT32 IHal_Convert_Tile420ToNV12(struct tile420_fmt src_buf, struct nv12_fmt *dst_buf)
{
	if (!dst_buf)
		return -IHAL_RERR;

	_tile420_nv12_y_msa(src_buf, dst_buf);
	_tile420_nv12_uv_msa(src_buf, dst_buf);

	return IHAL_ROK;
}
