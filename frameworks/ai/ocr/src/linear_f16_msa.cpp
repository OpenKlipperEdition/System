#include <cmath>
#include <cstring>
#include <type_traits>
#include "layers/layer.h"
#include "layers/linear_f16_msa.h"
#if defined(__mips_msa)
#include <msa.h>

#define pack_and_add(a0, a1, a2, a3, bias, result)              \
  do{                                                           \
    v4i32 pack_0, pack_1, pack_2, pack_3;                       \
                                                                \
    pack_0 = __msa_pckev_d((v4i32)a1, (v4i32) a0);                             \
    pack_1 = __msa_pckod_d((v4i32)a1, (v4i32) a0);                             \
    pack_2 = __msa_pckev_d((v4i32)a3, (v4i32) a2);                             \
    pack_3 = __msa_pckod_d((v4i32)a3, (v4i32) a2);                             \
    pack_1 = (v4i32)__msa_fadd_w((v4f32)pack_1, (v4f32)pack_0); \
    pack_3 = (v4i32)__msa_fadd_w((v4f32)pack_3, (v4f32)pack_2); \
    pack_0 = __msa_pckev_w(pack_3, pack_1);                     \
    pack_1 = __msa_pckod_w(pack_3, pack_1);                     \
                                                                \
    result = __msa_fadd_w((v4f32)pack_0, (v4f32)pack_1) + bias; \
  }while(0)

namespace tnn {
typedef union Cv32suf
{
  int i;
  unsigned u;
  float f;
} Cv32suf;

uint16_t Linear_f16_msa::cvt32fto16f(float x) const
{
  uint16_t w;
  Cv32suf in;
  in.f = x;
  unsigned sign = in.u & 0x80000000;
  in.u ^= sign;

  if( in.u >= 0x47800000 )
    w = (ushort)(in.u > 0x7f800000 ? 0x7e00 : 0x7c00);
  else
  {
    if (in.u < 0x38800000)
    {
      in.f += 0.5f;
      w = (ushort)(in.u - 0x3f000000);
    }
    else
    {
      unsigned t = in.u + 0xc8000fff;
      w = (ushort)((t + ((in.u >> 13) & 1)) >> 13);
    }
  }

  w = (uint16_t)(w | (sign >> 16));
  return w;
}

void Linear_f16_msa::load(std::istream &in) {

  tensor_float weight(m_weight.shape());
  weight.load(in);
  for(std::size_t i = 0;i < m_weight.size();i++){
    m_weight.at(i) = cvt32fto16f(weight.at(i));
  }
  m_bias.load(in);

};

tensorX Linear_f16_msa::forward(tensorX &&x, thread_pool &threads,std::shared_ptr<TensorParams> param) const {
  tensor_uint16 tensor;
  x.getTensor(tensor);

  tensor_float y{tensor.shape(0), tensor.shape(1),szOut};

  single_linear(tensor,y,0,tensor.shape(1));

  return y;
}

void Linear_f16_msa::single_linear(const tensor_uint16 &in, tensor_float &out,std::size_t s, std::size_t e) const
{
  const uint16_t *fin = in.get_raw();
  float *fout = out.get_raw();
  const uint16_t* weight = m_weight.get_raw();
  const float* bias = m_bias.get_raw();
  unsigned int width = in.shape(2);
  assert(width%8 == 0);

  std::size_t n = 0;
  for(n = 0;n < szOut/4*4;n += 4) {
    std::size_t ch;
    for(ch = s;ch < e/4*4;ch+=4) {
      // [ch][n]
      v4f32 vsum[4][4];
      vsum[0][0] = (v4f32)__msa_ldi_w(0);
      vsum[1][0] = vsum[0][0];
      vsum[2][0] = vsum[0][0];
      vsum[3][0] = vsum[0][0];

      vsum[0][1] = vsum[0][0]; vsum[0][2] = vsum[0][0];  vsum[0][3] = vsum[0][0];
      vsum[1][1] = vsum[0][0]; vsum[1][2] = vsum[0][0];  vsum[1][3] = vsum[0][0];
      vsum[2][1] = vsum[0][0]; vsum[2][2] = vsum[0][0];  vsum[2][3] = vsum[0][0];
      vsum[3][1] = vsum[0][0]; vsum[3][2] = vsum[0][0];  vsum[3][3] = vsum[0][0];

      for(std::size_t i = 0;i < width/8*8;i += 8) {
        v8i16 v_in_16[4];
        v_in_16[0]= __msa_ld_h((void*)(fin + (ch+0) * width + i), 0);
        v_in_16[1]= __msa_ld_h((void*)(fin + (ch+1) * width + i), 0);
        v_in_16[2]= __msa_ld_h((void*)(fin + (ch+2) * width + i), 0);
        v_in_16[3]= __msa_ld_h((void*)(fin + (ch+3) * width + i), 0);

        v8i16 v_wght_16[4];
        v_wght_16[0] = __msa_ld_h((void*)(weight + (n + 0) * width + i), 0);
        v_wght_16[1] = __msa_ld_h((void*)(weight + (n + 1) * width + i), 0);
        v_wght_16[2] = __msa_ld_h((void*)(weight + (n + 2) * width + i), 0);
        v_wght_16[3] = __msa_ld_h((void*)(weight + (n + 3) * width + i), 0);

        v4f32 v_in_f[4][2], v_wght_f[4][2];
        v_wght_f[0][0] = __msa_fexupr_w(v_wght_16[0]);
        v_wght_f[1][0] = __msa_fexupr_w(v_wght_16[1]);
        v_wght_f[2][0] = __msa_fexupr_w(v_wght_16[2]);
        v_wght_f[3][0] = __msa_fexupr_w(v_wght_16[3]);

        v_wght_f[0][1] = __msa_fexupl_w(v_wght_16[0]);
        v_wght_f[1][1] = __msa_fexupl_w(v_wght_16[1]);
        v_wght_f[2][1] = __msa_fexupl_w(v_wght_16[2]);
        v_wght_f[3][1] = __msa_fexupl_w(v_wght_16[3]);

        v_in_f[0][0] = __msa_fexupr_w(v_in_16[0]);
        v_in_f[1][0] = __msa_fexupr_w(v_in_16[1]);
        v_in_f[2][0] = __msa_fexupr_w(v_in_16[2]);
        v_in_f[3][0] = __msa_fexupr_w(v_in_16[3]);
#undef ALTER_FMADD_4
#define ALTER_FMADD_4(_a, _b, _c, ia0, ia1, ib0, ib1, ic0, ic1)         \
        do{                                                             \
          _a[ia0+0][ia1] = __msa_fmadd_w(_b[ib0+0][ib1], _c[ic0][ic1], _a[ia0+0][ia1]); \
          _a[ia0+1][ia1] = __msa_fmadd_w(_b[ib0+1][ib1], _c[ic0][ic1], _a[ia0+1][ia1]); \
          _a[ia0+2][ia1] = __msa_fmadd_w(_b[ib0+2][ib1], _c[ic0][ic1], _a[ia0+2][ia1]); \
          _a[ia0+3][ia1] = __msa_fmadd_w(_b[ib0+3][ib1], _c[ic0][ic1], _a[ia0+3][ia1]); \
        }while(0)

        ALTER_FMADD_4(vsum, v_in_f, v_wght_f, 0,0, 0,0, 0,0);
        ALTER_FMADD_4(vsum, v_in_f, v_wght_f, 0,1, 0,0, 1,0);
        ALTER_FMADD_4(vsum, v_in_f, v_wght_f, 0,2, 0,0, 2,0);
        ALTER_FMADD_4(vsum, v_in_f, v_wght_f, 0,3, 0,0, 3,0);

        v_in_f[0][1] = __msa_fexupl_w(v_in_16[0]);
        v_in_f[1][1] = __msa_fexupl_w(v_in_16[1]);
        v_in_f[2][1] = __msa_fexupl_w(v_in_16[2]);
        v_in_f[3][1] = __msa_fexupl_w(v_in_16[3]);

        ALTER_FMADD_4(vsum, v_in_f, v_wght_f, 0,0, 0,1, 0,1);
        ALTER_FMADD_4(vsum, v_in_f, v_wght_f, 0,1, 0,1, 1,1);
        ALTER_FMADD_4(vsum, v_in_f, v_wght_f, 0,2, 0,1, 2,1);
        ALTER_FMADD_4(vsum, v_in_f, v_wght_f, 0,3, 0,1, 3,1);

      }

      v4f32 e_01, e_23;
      v4f32 vbias;
      vbias =(v4f32) __msa_ld_w((void*)(bias+n), 0);

      v4f32 result[8];
      pack_and_add(vsum[0][0], vsum[0][1], vsum[0][2], vsum[0][3], vbias, result[0]);
      pack_and_add(vsum[1][0], vsum[1][1], vsum[1][2], vsum[1][3], vbias, result[1]);
      pack_and_add(vsum[2][0], vsum[2][1], vsum[2][2], vsum[2][3], vbias, result[2]);
      pack_and_add(vsum[3][0], vsum[3][1], vsum[3][2], vsum[3][3], vbias, result[3]);

      __msa_st_w((v4i32)result[0], fout + ch * szOut + n, 0);
      __msa_st_w((v4i32)result[1], fout + (ch+1) * szOut + n, 0);
      __msa_st_w((v4i32)result[2], fout + (ch+2) * szOut + n, 0);
      __msa_st_w((v4i32)result[3], fout + (ch+3) * szOut + n, 0);
    }

    // for ch unalign
    for(;ch < e;ch+=1){
      v4f32 vsum[4];
      vsum[0] = (v4f32)__msa_ldi_w(0);
      vsum[1] = vsum[0]; vsum[2] = vsum[0]; vsum[3] = vsum[0];

      for(std::size_t i = 0;i < width/8*8;i += 8) {
        v8i16 v_in_16;
        v_in_16 = __msa_ld_h((void*)(fin + (ch+0) * width + i), 0);

        v8i16 v_wght_16[4];
        v_wght_16[0] = __msa_ld_h((void*)(weight + (n + 0) * width + i), 0);
        v_wght_16[1] = __msa_ld_h((void*)(weight + (n + 1) * width + i), 0);
        v_wght_16[2] = __msa_ld_h((void*)(weight + (n + 2) * width + i), 0);
        v_wght_16[3] = __msa_ld_h((void*)(weight + (n + 3) * width + i), 0);

        v4f32 v_in_f[2], v_wght_f[4][2];
        v_wght_f[0][0] = __msa_fexupr_w(v_wght_16[0]);
        v_wght_f[1][0] = __msa_fexupr_w(v_wght_16[1]);
        v_wght_f[2][0] = __msa_fexupr_w(v_wght_16[2]);
        v_wght_f[3][0] = __msa_fexupr_w(v_wght_16[3]);

        v_wght_f[0][1] = __msa_fexupl_w(v_wght_16[0]);
        v_wght_f[1][1] = __msa_fexupl_w(v_wght_16[1]);
        v_wght_f[2][1] = __msa_fexupl_w(v_wght_16[2]);
        v_wght_f[3][1] = __msa_fexupl_w(v_wght_16[3]);

        v_in_f[0] = __msa_fexupr_w(v_in_16);
        v_in_f[1] = __msa_fexupl_w(v_in_16);

#undef ALTER_FMADD_4
#define ALTER_FMADD_4(_a, _b, _c, ia0, ib0, ic0, ic1)                   \
        do{                                                             \
          _a[ia0+0] = __msa_fmadd_w(_b[ib0], _c[ic0+0][ic1], _a[ia0+0]); \
          _a[ia0+1] = __msa_fmadd_w(_b[ib0], _c[ic0+1][ic1], _a[ia0+1]); \
          _a[ia0+2] = __msa_fmadd_w(_b[ib0], _c[ic0+2][ic1], _a[ia0+2]); \
          _a[ia0+3] = __msa_fmadd_w(_b[ib0], _c[ic0+3][ic1], _a[ia0+3]); \
        }while(0)

        ALTER_FMADD_4(vsum, v_in_f, v_wght_f, 0, 0, 0,0);
        ALTER_FMADD_4(vsum, v_in_f, v_wght_f, 0, 1, 0,1);
      }

      v4f32 e_01, e_23;
      v4f32 vbias;
      vbias =(v4f32) __msa_ld_w((void*)(bias+n), 0);
      v4f32 result[8];
      pack_and_add(vsum[0], vsum[1], vsum[2], vsum[3], vbias, result[0]);
      __msa_st_w((v4i32)result[0], fout + ch * szOut + n, 0);
    }
  }

  // for szOut unalign
  for(;n < szOut;n += 1) {
    std::size_t ch;
    for(ch = s;ch < e/4*4;ch+=4) {
      // [ch][n]
      v4f32 vsum[4];
      vsum[0] = (v4f32)__msa_ldi_w(0);
      vsum[1] = vsum[0];
      vsum[2] = vsum[0];
      vsum[3] = vsum[0];

      for(std::size_t i = 0;i < width/8*8;i += 8) {
        v8i16 v_in_16[4];
        v_in_16[0]= __msa_ld_h((void*)(fin + (ch+0) * width + i), 0);
        v_in_16[1]= __msa_ld_h((void*)(fin + (ch+1) * width + i), 0);
        v_in_16[2]= __msa_ld_h((void*)(fin + (ch+2) * width + i), 0);
        v_in_16[3]= __msa_ld_h((void*)(fin + (ch+3) * width + i), 0);

        v8i16 v_wght_16;
        v_wght_16 = __msa_ld_h((void*)(weight + (n + 0) * width + i), 0);

        v4f32 v_in_f[4][2], v_wght_f[2];
        v_wght_f[0] = __msa_fexupr_w(v_wght_16);
        v_wght_f[1] = __msa_fexupl_w(v_wght_16);

        v_in_f[0][0] = __msa_fexupr_w(v_in_16[0]);
        v_in_f[1][0] = __msa_fexupr_w(v_in_16[1]);
        v_in_f[2][0] = __msa_fexupr_w(v_in_16[2]);
        v_in_f[3][0] = __msa_fexupr_w(v_in_16[3]);
        v_in_f[0][1] = __msa_fexupl_w(v_in_16[0]);
        v_in_f[1][1] = __msa_fexupl_w(v_in_16[1]);
        v_in_f[2][1] = __msa_fexupl_w(v_in_16[2]);
        v_in_f[3][1] = __msa_fexupl_w(v_in_16[3]);

#undef ALTER_FMADD_4
#define ALTER_FMADD_4(_a, _b, _c, ia0, ib0, ib1, ic0)                   \
        do{                                                             \
          _a[ia0+0] = __msa_fmadd_w(_b[ib0+0][ib1], _c[ic0], _a[ia0+0]); \
          _a[ia0+1] = __msa_fmadd_w(_b[ib0+1][ib1], _c[ic0], _a[ia0+1]); \
          _a[ia0+2] = __msa_fmadd_w(_b[ib0+2][ib1], _c[ic0], _a[ia0+2]); \
          _a[ia0+3] = __msa_fmadd_w(_b[ib0+3][ib1], _c[ic0], _a[ia0+3]); \
        }while(0)

        ALTER_FMADD_4(vsum, v_in_f, v_wght_f, 0, 0,0, 0);
        ALTER_FMADD_4(vsum, v_in_f, v_wght_f, 0, 0,1, 1);
      }
      float fbias = bias[n];
      v4f32 result;
      v4f32 vbias = (v4f32)__msa_fill_w(*((int*)&fbias));
      pack_and_add(vsum[0], vsum[1], vsum[2], vsum[3], vbias, result);

      fout[ch * szOut + n] = result[0];
      fout[(ch+1) * szOut + n] = result[1];
      fout[(ch+2) * szOut + n] = result[2];
      fout[(ch+3) * szOut + n] = result[3];
    }


    for(;ch < e;ch+=1) {
      // [left/right]
      v4f32 vsum[2];
      vsum[0] = (v4f32)__msa_ldi_w(0);
      vsum[1] = vsum[0];

      for(std::size_t i = 0;i < width/8*8;i += 8) {
        v8i16 v_in_16, v_wght_16;
        v_in_16 = __msa_ld_h((void*)(fin + (ch+0) * width + i), 0);
        v_wght_16 = __msa_ld_h((void*)(weight + (n + 0) * width + i), 0);

        v4f32 v_in_f[2], v_wght_f[2];
        v_wght_f[0] = __msa_fexupr_w(v_wght_16);
        v_wght_f[1] = __msa_fexupl_w(v_wght_16);
        v_in_f[0] = __msa_fexupr_w(v_in_16);
        v_in_f[1] = __msa_fexupl_w(v_in_16);

        vsum[0] = __msa_fmadd_w(v_in_f[0], v_wght_f[0], vsum[0]);
        vsum[1] = __msa_fmadd_w(v_in_f[1], v_wght_f[1], vsum[1]);
      }
      float fbias = bias[n];
      v4f32 result = vsum[0] + vsum[1];
      fout[ch * szOut + n] = fbias + result[0] + result[1] + result[2] + result[3];
    }
  }
}

}
#else
#error unsupport MIPS MSA compile for MSA optimized LSTM.
#endif
