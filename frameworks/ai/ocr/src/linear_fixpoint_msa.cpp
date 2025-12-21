#include <cmath>
#include <cstring>
#include <type_traits>
#include "layers/layer.h"
#include "layers/linear_fixpoint_msa.h"
#if defined(__mips_msa)
#include <msa.h>

#define pack_and_add_integer(a0, a1, a2, a3, bias, scale, result)       \
  do{                                                                   \
    v4i32 pack_0, pack_1, pack_2, pack_3;                               \
                                                                        \
    pack_0 = __msa_pckev_d((v4i32)a1, (v4i32) a0);                      \
    pack_1 = __msa_pckod_d((v4i32)a1, (v4i32) a0);                      \
    pack_2 = __msa_pckev_d((v4i32)a3, (v4i32) a2);                      \
    pack_3 = __msa_pckod_d((v4i32)a3, (v4i32) a2);                      \
    pack_1 = pack_1 + pack_0;                                           \
    pack_3 = pack_3 + pack_2;                                           \
    pack_0 = __msa_pckev_w(pack_3, pack_1);                             \
    pack_1 = __msa_pckod_w(pack_3, pack_1);                             \
                                                                        \
    result = (v4f32)(pack_0 + pack_1);                                  \
    result = __msa_ffint_s_w((v4i32)result);                            \
    result = result*scale  + bias;                                      \
  }while(0)

#define ALTER_EXTEND_ADD_4_2GROUP(_a, _b, _c, ia0, ia1, ib0, ic0)       \
  do{                                                                   \
    v8i16 _vt[4][2];                                                    \
    _vt[0][0] = __msa_dotp_s_h(_b[ib0+0], _c[ic0]);                 \
    _vt[1][0] = __msa_dotp_s_h(_b[ib0+1], _c[ic0]);                 \
    _vt[2][0] = __msa_dotp_s_h(_b[ib0+2], _c[ic0]);                 \
    _vt[3][0] = __msa_dotp_s_h(_b[ib0+3], _c[ic0]);                 \
                                                                        \
    _vt[0][1] = __msa_dotp_s_h(_b[ib0+0], _c[ic0+1]);             \
    _vt[1][1] = __msa_dotp_s_h(_b[ib0+1], _c[ic0+1]);             \
    _vt[2][1] = __msa_dotp_s_h(_b[ib0+2], _c[ic0+1]);             \
    _vt[3][1] = __msa_dotp_s_h(_b[ib0+3], _c[ic0+1]);             \
                                                                        \
    _a[ia0+0][ia1] = __msa_dpadd_s_w(_a[ia0+0][ia1], _vt[0][0], one);   \
    _a[ia0+1][ia1] = __msa_dpadd_s_w(_a[ia0+1][ia1], _vt[1][0], one);   \
    _a[ia0+2][ia1] = __msa_dpadd_s_w(_a[ia0+2][ia1], _vt[2][0], one);   \
    _a[ia0+3][ia1] = __msa_dpadd_s_w(_a[ia0+3][ia1], _vt[3][0], one);   \
                                                                        \
    _a[ia0+0][ia1+1] = __msa_dpadd_s_w(_a[ia0+0][ia1+1], _vt[0][1], one); \
    _a[ia0+1][ia1+1] = __msa_dpadd_s_w(_a[ia0+1][ia1+1], _vt[1][1], one); \
    _a[ia0+2][ia1+1] = __msa_dpadd_s_w(_a[ia0+2][ia1+1], _vt[2][1], one); \
    _a[ia0+3][ia1+1] = __msa_dpadd_s_w(_a[ia0+3][ia1+1], _vt[3][1], one); \
  }while(0)

namespace tnn {
typedef union Cv32suf
{
  int i;
  unsigned u;
  float f;
} Cv32suf;

uint16_t Linear_fixpoint_msa::cvt32fto16f(float x) const
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

float Linear_fixpoint_msa::cvt16fto32f(uint16_t w) const
{

  Cv32suf out;

  unsigned t = ((w & 0x7fff) << 13) + 0x38000000;
  unsigned sign = (w & 0x8000) << 16;
  unsigned e = w & 0x7c00;

  out.u = t + (1 << 23);
  out.u = (e >= 0x7c00 ? t + 0x38000000 :
           e == 0 ? (static_cast<void>(out.f -= 6.103515625e-05f), out.u) : t) | sign;
  return out.f;
}

void Linear_fixpoint_msa::load(std::istream &in) {

    tensor_float weight(m_weight.shape());
    factor_m_weight = tensor_float({szOut});

    weight.load(in);
    for(std::size_t ch = 0; ch < szOut; ch++) {
      float fmax = 0;
      for(std::size_t w = 0; w < 2*szHidden; w++) {
        float f = fabs(weight.at(ch, w));
        if (f > fmax)
          fmax = f;
      }
      float scale = 127.0/fmax;
      for(std::size_t w = 0; w < 2*szHidden; w++) {
        m_weight_8.at(ch, w) = scale * weight.at(ch,w);
      }
      factor_m_weight.at(ch) = 1.0/scale;
    }

    for(std::size_t i = 0;i < m_weight.size();i++){
      m_weight.at(i) = cvt32fto16f(weight.at(i));
    }
    m_bias.load(in);
  };

tensorX Linear_fixpoint_msa::forward(tensorX &&x, thread_pool &threads,std::shared_ptr<TensorParams> param) const {
  tensor_uint16 tensor;
  x.getTensor(tensor);

  tensor_float y{tensor.shape(0), tensor.shape(1),szOut};

  single_linear(tensor,y,0,tensor.shape(1));

  return y;
}

#pragma GCC optimize ("-fno-schedule-insns,-fno-schedule-insns2")
void Linear_fixpoint_msa::single_linear(const tensor_uint16 &in, tensor_float &out,std::size_t s, std::size_t e) const
{
  const uint16_t *fin = in.get_raw();
  float *fout = out.get_raw();
  const uint16_t* weight = m_weight.get_raw();
  const float* bias = m_bias.get_raw();
  tensor_float factor_in({e});
  tensor_int8 in_8(in.shape());
  int width = in.shape(2);
  long long start, finish, finish2;

  for(std::size_t ch = s;ch < e;ch++) {
    float fmax = 0;
    for(std::size_t i = 0;i < width;i++) {
      float f = cvt16fto32f(fin[ch * width + i]);
      f = fabs(f);
      if (f > fmax)
        fmax = f;
    }
    float scale = 127.0/fmax;
    for(std::size_t i = 0;i < width;i++) {
      float f = cvt16fto32f(fin[ch * width + i]);
      in_8.at(ch, i) = f * scale;
    }
    factor_in.at(ch) = 1.0/scale;
  }

  int8_t *in8_p = (int8_t*)in_8.get_raw();
  int8_t *weight8_p = (int8_t*)m_weight_8.get_raw();
  float *factor_in_p = (float*)factor_in.get_raw();
  float *factor_wght_p = (float*)factor_m_weight.get_raw();

  v8i16 one = __msa_ldi_h(1);
  std::size_t n = 0;
  for(n = 0;n < szOut/4*4;n+=4) {
    std::size_t ch = s;
    for(ch = s;ch < e/4*4;ch+=4) {
      // [ch][n]
      v4i32 vsum[4][4];
      vsum[0][0] = (v4i32)__msa_ldi_w(0);
      vsum[1][0] = vsum[0][0];
      vsum[2][0] = vsum[0][0];
      vsum[3][0] = vsum[0][0];

      vsum[0][1] = vsum[0][0]; vsum[0][2] = vsum[0][0];  vsum[0][3] = vsum[0][0];
      vsum[1][1] = vsum[0][0]; vsum[1][2] = vsum[0][0];  vsum[1][3] = vsum[0][0];
      vsum[2][1] = vsum[0][0]; vsum[2][2] = vsum[0][0];  vsum[2][3] = vsum[0][0];
      vsum[3][1] = vsum[0][0]; vsum[3][2] = vsum[0][0];  vsum[3][3] = vsum[0][0];

      for(std::size_t i = 0;i < width/16*16;i+=16) {
        v16i8 v_in[4], v_wght[4];

        v_in[0] = __msa_ld_w((void*)(in8_p + ch * width + i), 0);
        v_in[1] = __msa_ld_w((void*)(in8_p + (ch+1) * width + i), 0);
        v_in[2] = __msa_ld_w((void*)(in8_p + (ch+2) * width + i), 0);
        v_in[3] = __msa_ld_w((void*)(in8_p + (ch+3) * width + i), 0);

        v_wght[0] = __msa_ld_w((void*)(weight8_p + (n+0) * width + i), 0);
        v_wght[1] = __msa_ld_w((void*)(weight8_p + (n+1) * width + i), 0);
        v_wght[2] = __msa_ld_w((void*)(weight8_p + (n+2) * width + i), 0);
        v_wght[3] = __msa_ld_w((void*)(weight8_p + (n+3) * width + i), 0);

        ALTER_EXTEND_ADD_4_2GROUP(vsum, v_in, v_wght, 0,0, 0, 0);
        ALTER_EXTEND_ADD_4_2GROUP(vsum, v_in, v_wght, 0,2, 0, 2);
      }
      v4f32 vbias;

      v4f32 scale_in, scale_w, scale[4];
      scale_in =(v4f32) __msa_ld_w(factor_in_p + ch, 0);
      scale_w  =(v4f32) __msa_ld_w(factor_wght_p + n, 0);
      vbias =(v4f32) __msa_ld_w((void*)(bias+n), 0);

      scale[0] =(v4f32) __msa_splati_w((v4i32)scale_in, 0);
      scale[1] =(v4f32) __msa_splati_w((v4i32)scale_in, 1);
      scale[2] =(v4f32) __msa_splati_w((v4i32)scale_in, 2);
      scale[3] =(v4f32) __msa_splati_w((v4i32)scale_in, 3);

      scale[0] = scale[0] * scale_w;
      scale[1] = scale[1] * scale_w;
      scale[2] = scale[2] * scale_w;
      scale[3] = scale[3] * scale_w;
      v4f32 result[8];
      pack_and_add_integer(vsum[0][0], vsum[0][1], vsum[0][2], vsum[0][3], vbias, scale[0], result[0]);
      pack_and_add_integer(vsum[1][0], vsum[1][1], vsum[1][2], vsum[1][3], vbias, scale[1], result[1]);
      pack_and_add_integer(vsum[2][0], vsum[2][1], vsum[2][2], vsum[2][3], vbias, scale[2], result[2]);
      pack_and_add_integer(vsum[3][0], vsum[3][1], vsum[3][2], vsum[3][3], vbias, scale[3], result[3]);

      __msa_st_w((v4i32)result[0], fout + ch * szOut + n, 0);
      __msa_st_w((v4i32)result[1], fout + (ch+1) * szOut + n, 0);
      __msa_st_w((v4i32)result[2], fout + (ch+2) * szOut + n, 0);
      __msa_st_w((v4i32)result[3], fout + (ch+3) * szOut + n, 0);
    }

    for(;ch < e;ch+=1) {
      // [ch][n]
      v4i32 vsum[4];
      vsum[0] = (v4i32)__msa_ldi_w(0);
      vsum[1] = vsum[0];
      vsum[2] = vsum[0];
      vsum[3] = vsum[0];

      for(std::size_t i = 0;i < width/16*16;i+=16) {
        v16i8 v_in[4], v_wght[4];
        v_in[0] = __msa_ld_w((void*)(in8_p + ch * width + i), 0);
        v_wght[0] = __msa_ld_w((void*)(weight8_p + (n+0) * width + i), 0);
        v_wght[1] = __msa_ld_w((void*)(weight8_p + (n+1) * width + i), 0);
        v_wght[2] = __msa_ld_w((void*)(weight8_p + (n+2) * width + i), 0);
        v_wght[3] = __msa_ld_w((void*)(weight8_p + (n+3) * width + i), 0);

        v8i16 vt[4];
        vt[0] = __msa_dotp_s_h(v_in[0], v_wght[0]);
        vt[1] = __msa_dotp_s_h(v_in[0], v_wght[1]);
        vt[2] = __msa_dotp_s_h(v_in[0], v_wght[2]);
        vt[3] = __msa_dotp_s_h(v_in[0], v_wght[3]);

        vsum[0] = __msa_dpadd_s_w(vsum[0], vt[0], one);
        vsum[1] = __msa_dpadd_s_w(vsum[1], vt[1], one);
        vsum[2] = __msa_dpadd_s_w(vsum[2], vt[2], one);
        vsum[3] = __msa_dpadd_s_w(vsum[3], vt[3], one);
      }
      v4f32 vbias, result, scale_w, scale;
      float fscale_in = factor_in_p[ch];
      scale_w  =(v4f32) __msa_ld_w(factor_wght_p + n, 0);
      vbias =(v4f32) __msa_ld_w((void*)(bias+n), 0);
      scale =(v4f32) __msa_fill_w(*((int*)&fscale_in));
      scale = scale * scale_w;

      pack_and_add_integer(vsum[0], vsum[1], vsum[2], vsum[3], vbias, scale, result);
      __msa_st_w((v4i32)result, fout + ch * szOut + n, 0);
    } // end e unalign
  } // szOout align

  for(;n < szOut;n+=1) {   // szOut unalign
    std::size_t ch = s;
    for(ch = s;ch < e/4*4;ch+=4) {
      // [ch][n]
      v4i32 vsum[4];
      vsum[0] = (v4i32)__msa_ldi_w(0);
      vsum[1] = vsum[0];
      vsum[2] = vsum[0];
      vsum[3] = vsum[0];

      for(std::size_t i = 0;i < width/16*16;i+=16) {
        v16i8 v_in[4], v_wght;
        v_in[0] = __msa_ld_w((void*)(in8_p + ch * width + i), 0);
        v_in[1] = __msa_ld_w((void*)(in8_p + (ch+1) * width + i), 0);
        v_in[2] = __msa_ld_w((void*)(in8_p + (ch+2) * width + i), 0);
        v_in[3] = __msa_ld_w((void*)(in8_p + (ch+3) * width + i), 0);
        v_wght = __msa_ld_w((void*)(weight8_p + (n+0) * width + i), 0);

        v8i16 vt[4];
        vt[0] = __msa_dotp_s_h(v_in[0], v_wght);
        vt[1] = __msa_dotp_s_h(v_in[1], v_wght);
        vt[2] = __msa_dotp_s_h(v_in[2], v_wght);
        vt[3] = __msa_dotp_s_h(v_in[3], v_wght);

        vsum[0] = __msa_dpadd_s_w(vsum[0], vt[0], one);
        vsum[1] = __msa_dpadd_s_w(vsum[1], vt[1], one);
        vsum[2] = __msa_dpadd_s_w(vsum[2], vt[2], one);
        vsum[3] = __msa_dpadd_s_w(vsum[3], vt[3], one);
      }
      v4f32 vbias, scale_in, scale, result;
      float fscale_w = factor_wght_p[n];
      float fbias = bias[n];
      scale_in =(v4f32) __msa_ld_w(factor_in_p + ch, 0);
      vbias =(v4f32) __msa_fill_w((*(int*)&fbias));
      scale =(v4f32) __msa_fill_w((*(int*)&fscale_w));
      scale = scale * scale_in;
      pack_and_add_integer(vsum[0], vsum[1], vsum[2], vsum[3], vbias, scale, result);
      fout[ch * szOut + n] = result[0];
      fout[(ch+1) * szOut + n] = result[1];
      fout[(ch+2) * szOut + n] = result[2];
      fout[(ch+3) * szOut + n] = result[3];
    }

    for(;ch < e;ch+=1) {
      v4i32 vsum;
      vsum = (v4i32)__msa_ldi_w(0);
      for(std::size_t i = 0;i < width/16*16;i+=16) {
        v16i8 v_in, v_wght;
        v_in = __msa_ld_w((void*)(in8_p + ch * width + i), 0);
        v_wght = __msa_ld_w((void*)(weight8_p + (n+0) * width + i), 0);
        v8i16 vt;
        vt = __msa_dotp_s_h(v_in, v_wght);
        vsum = __msa_dpadd_s_w(vsum, vt, one);
      }
      float fscale_in = factor_in_p[ch];
      float fscale_w = factor_wght_p[n];
      float fbias = bias[n];
      float result = (vsum[0] + vsum[1] + vsum[2] + vsum[3]) * fscale_w * fscale_in + fbias;
      fout[ch*szOut + n] = result;
    } // end e unalign
  } // end szOut unalign
}
}

#else
#error unsupport MIPS MSA compile for MSA optimized LSTM.
#endif
