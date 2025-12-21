#include <cmath>
#include <cstring>
#include <type_traits>
#include "layers/layer.h"
#include "layers/bidirectionalLSTM_f16_msa.h"
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

// (1+x/n)^n
#define get_neg_exp(x, neg_factor)                      \
  ({                                                    \
    float fone = 1.0;                                   \
    v4f32 one =(v4f32) __msa_fill_w(*(int*)(&fone));    \
    v4i32 _t = (v4i32) __msa_ldi_w(-8);                 \
    neg_factor = __msa_fexp2_w(x, _t);                  \
    neg_factor = one - neg_factor;                      \
                                                        \
    neg_factor = __msa_fmul_w(neg_factor, neg_factor);  \
    neg_factor = __msa_fmul_w(neg_factor, neg_factor);  \
    neg_factor = __msa_fmul_w(neg_factor, neg_factor);  \
    neg_factor = __msa_fmul_w(neg_factor, neg_factor);  \
    neg_factor = __msa_fmul_w(neg_factor, neg_factor);  \
    neg_factor = __msa_fmul_w(neg_factor, neg_factor);  \
    neg_factor = __msa_fmul_w(neg_factor, neg_factor);  \
    neg_factor = __msa_fmul_w(neg_factor, neg_factor);  \
  })

#define get_sigmod(x_exp, result)                       \
  do {                                                  \
    float fone = 1.0;                                   \
    v4f32 one = (v4f32)__msa_fill_w(*(int*)(&fone));    \
    result = x_exp + one;                               \
    result = __msa_frcp_w(result);                      \
  }while(0)

// 1/(1+e^(-2x)) - 1/(1+e^(2x))
#define get_tan(x, result)                      \
  do {                                          \
    v4f32 _nexp, _exp;                          \
    get_neg_exp(x, _nexp);                      \
    _nexp = _nexp * _nexp;                      \
    v4f32 op1, op2;                             \
    get_sigmod(_nexp, op1);                     \
    _exp = __msa_frcp_w(_nexp);                 \
    get_sigmod(_exp, op2);                      \
    result = op1 - op2;                         \
  }while(0)

namespace tnn {
typedef union Cv32suf
{
  int i;
  unsigned u;
  float f;
} Cv32suf;

uint16_t BidirectionalLSTM_f16_msa::cvt32fto16f(float x) const
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

float BidirectionalLSTM_f16_msa::cvt16fto32f(uint16_t w) const
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

inline float sigmoid(float x){
  return 1.0/(1.0 + std::exp(-x));
}

void BidirectionalLSTM_f16_msa::load(std::istream &in) {

  for(std::size_t i = 0;i < params.size();i++){
    tensor_float ih(params[i]->w_ih.shape());
    tensor_float hh(params[i]->w_hh.shape());
    ih.load(in);
    hh.load(in);
    for(std::size_t j = 0;j < ih.size();j++){
      params[i]->w_ih.at(j) = cvt32fto16f(ih.at(j));
    }
    for(std::size_t j = 0;j < hh.size();j++){
      params[i]->w_hh.at(j) = cvt32fto16f(hh.at(j));
    }
    params[i]->b_ih.load(in);
    params[i]->b_hh.load(in);
  }
};

tensorX BidirectionalLSTM_f16_msa::forward(tensorX &&x, thread_pool &threads,std::shared_ptr<TensorParams> param) const {
  tensor_float tensor = x.getTensor();
  assert(tensor.ndim() == 3 && tensor.shape(2) == szInput);

  tensor_float in = tensor;

  tensor_uint16 in_16(in.shape());
  tensor_uint16 out_16{tensor.shape(0),tensor.shape(1),szHidden * 2};

  uint16_t *in_16_p = in_16.get_raw();
  float *in_p = in.get_raw();

  for(std::size_t i = 0;i < in_16.size()/8*8; i+=8){
    v4f32 v_in_f[2];
    v_in_f[0] = (v4f32)__msa_ld_b(in_p + i, 0);
    v_in_f[1] = (v4f32)__msa_ld_b(in_p + i, 16);
    v8u16 v_in_16;
    v_in_16 = __msa_fexdo_h(v_in_f[1], v_in_f[0]);
    __msa_st_w(v_in_16, in_16_p + i, 0);
  }

  single_lstm_fwd(in_16,out_16,params[0],0,0,tensor.shape(1));
  single_lstm_bwd(in_16,out_16,params[1],szHidden,0,tensor.shape(1));

  return out_16;
}

void BidirectionalLSTM_f16_msa::single_lstm_fwd(const tensor_uint16 &in, tensor_uint16 &out,const std::shared_ptr<Parameters> &p,std::size_t off,std::size_t s, std::size_t e) const
{
  std::size_t batch = 0;
  std::size_t chan_size = in.shape(1), width = in.shape(2);
  assert(width%8 == 0);
  std::size_t ig_off = 0 * szHidden;
  std::size_t fg_off = 1 * szHidden;
  std::size_t cell_off = 2 * szHidden;
  std::size_t out_off = 3 * szHidden;
  float input_gate = 0,forget_gate = 0,cell_gate = 0,out_gate = 0;

  tensor_int16 hx{szHidden};
  tensor_float cx{szHidden};
  tensor_float sum_tmp{e, szHidden*4};

  memset(hx.get_raw(),0,hx.size() * sizeof(short));
  memset(cx.get_raw(),0,cx.size() * sizeof(float));

  const uint16_t *w_ih = p->w_ih.get_raw();
  const uint16_t *w_hh = p->w_hh.get_raw();
  const uint16_t *indata = in.get_raw();
  long long start, step1 = 0, step2 = 0, finish_1, finish_2, step3 = 0;
  float* b_hh =  (float*)p->b_hh.get_raw();
  float* b_ih =  (float*)p->b_ih.get_raw();
  float *cx_p = cx.get_raw();
  short *hx_p = hx.get_raw();
  float *sum_p = sum_tmp.get_raw();
  uint16_t *out_p = out.get_raw();

    for(std::size_t h = 0;h < szHidden * 4/4*4 ; h += 4){
      std::size_t ch;
    for (ch = s; ch < e/4*4; ch +=4) {
      v4f32 vsum[4][4];
      vsum[0][0] = (v4f32)__msa_ldi_w(0);
      vsum[1][0] = vsum[0][0];
      vsum[2][0] = vsum[0][0];
      vsum[3][0] = vsum[0][0];

      vsum[0][1] = vsum[0][0]; vsum[0][2] = vsum[0][0]; vsum[0][3] = vsum[0][0];
      vsum[1][1] = vsum[0][0]; vsum[1][2] = vsum[0][0]; vsum[1][3] = vsum[0][0];
      vsum[2][1] = vsum[0][0]; vsum[2][2] = vsum[0][0]; vsum[2][3] = vsum[0][0];
      vsum[3][1] = vsum[0][0]; vsum[3][2] = vsum[0][0]; vsum[3][3] = vsum[0][0];

      uint16_t *in_base = (uint16_t *)(indata + ch * width);
      uint16_t *wih_base = (uint16_t *)(w_ih + h * width);

      for (std::size_t w = 0; w < width/8*8; w += 8) {
        v8u16 v_in_16[4], v_wih_16[4];
        v4f32 v_in_f[4][2], v_wih_f[4][2];

        v_in_16[0] = __msa_ld_b(in_base + w, 0);
        v_in_16[1] = __msa_ld_b(in_base + 1 * width + w, 0);
        v_in_16[2] = __msa_ld_b(in_base + 2 * width + w, 0);
        v_in_16[3] = __msa_ld_b(in_base + 3 * width + w, 0);
        v_wih_16[0] = __msa_ld_h (wih_base + w, 0);
        v_wih_16[1] = __msa_ld_h (wih_base + 1 * width + w, 0);
        v_wih_16[2] = __msa_ld_h (wih_base + 2 * width + w, 0);
        v_wih_16[3] = __msa_ld_h (wih_base + 3 * width + w, 0);

        v_wih_f[0][0] = __msa_fexupr_w(v_wih_16[0]);
        v_wih_f[1][0] = __msa_fexupr_w(v_wih_16[1]);
        v_wih_f[2][0] = __msa_fexupr_w(v_wih_16[2]);
        v_wih_f[3][0] = __msa_fexupr_w(v_wih_16[3]);
        v_wih_f[0][1] = __msa_fexupl_w(v_wih_16[0]);
        v_wih_f[1][1] = __msa_fexupl_w(v_wih_16[1]);
        v_wih_f[2][1] = __msa_fexupl_w(v_wih_16[2]);
        v_wih_f[3][1] = __msa_fexupl_w(v_wih_16[3]);

        v_in_f[0][0]= __msa_fexupr_w(v_in_16[0]);
        v_in_f[1][0]= __msa_fexupr_w(v_in_16[1]);
        v_in_f[2][0]= __msa_fexupr_w(v_in_16[2]);
        v_in_f[3][0]= __msa_fexupr_w(v_in_16[3]);

#define ALTER_FMADD_4(_a, _b, _c, ia0, ia1, ib0, ib1, ic0, ic1) \
        do{                                                             \
          _a[ia0+0][ia1] = __msa_fmadd_w(_b[ib0+0][ib1], _c[ic0][ic1], _a[ia0+0][ia1]); \
          _a[ia0+1][ia1] = __msa_fmadd_w(_b[ib0+1][ib1], _c[ic0][ic1], _a[ia0+1][ia1]); \
          _a[ia0+2][ia1] = __msa_fmadd_w(_b[ib0+2][ib1], _c[ic0][ic1], _a[ia0+2][ia1]); \
          _a[ia0+3][ia1] = __msa_fmadd_w(_b[ib0+3][ib1], _c[ic0][ic1], _a[ia0+3][ia1]); \
        }while(0)

        ALTER_FMADD_4(vsum, v_in_f, v_wih_f, 0,0, 0,0, 0,0);
        ALTER_FMADD_4(vsum, v_in_f, v_wih_f, 0,1, 0,0, 1,0);
        ALTER_FMADD_4(vsum, v_in_f, v_wih_f, 0,2, 0,0, 2,0);
        ALTER_FMADD_4(vsum, v_in_f, v_wih_f, 0,3, 0,0, 3,0);

        v_in_f[0][1]= __msa_fexupl_w(v_in_16[0]);
        v_in_f[1][1]= __msa_fexupl_w(v_in_16[1]);
        v_in_f[2][1]= __msa_fexupl_w(v_in_16[2]);
        v_in_f[3][1]= __msa_fexupl_w(v_in_16[3]);

        ALTER_FMADD_4(vsum, v_in_f, v_wih_f, 0,0, 0,1, 0,1);
        ALTER_FMADD_4(vsum, v_in_f, v_wih_f, 0,1, 0,1, 1,1);
        ALTER_FMADD_4(vsum, v_in_f, v_wih_f, 0,2, 0,1, 2,1);
        ALTER_FMADD_4(vsum, v_in_f, v_wih_f, 0,3, 0,1, 3,1);
      }

      v4f32 result[4];
      v4f32 v_bih, v_bhh ;
      v_bih =(v4f32) __msa_ld_w(b_ih + h, 0);
      v_bhh =(v4f32) __msa_ld_w(b_hh + h, 0);
      v_bih = v_bih + v_bhh;
      pack_and_add(vsum[0][0], vsum[0][1], vsum[0][2], vsum[0][3], v_bih, result[0]);
      pack_and_add(vsum[1][0], vsum[1][1], vsum[1][2], vsum[1][3], v_bih, result[1]);
      pack_and_add(vsum[2][0], vsum[2][1], vsum[2][2], vsum[2][3], v_bih, result[2]);
      pack_and_add(vsum[3][0], vsum[3][1], vsum[3][2], vsum[3][3], v_bih, result[3]);

      __msa_st_w((v4i32)result[0], sum_p + ch * 4 * szHidden + h, 0);
      __msa_st_w((v4i32)result[1], sum_p + (ch+1) * 4 * szHidden + h, 0);
      __msa_st_w((v4i32)result[2], sum_p + (ch+2) * 4 * szHidden + h, 0);
      __msa_st_w((v4i32)result[3], sum_p + (ch+3) * 4 * szHidden + h, 0);
    }

    // for e unalign
    for (; ch < e; ch +=1) {
      v4f32 vsum[4];
      vsum[0] = (v4f32)__msa_ldi_w(0);
      vsum[1] = vsum[0];
      vsum[2] = vsum[0];
      vsum[3] = vsum[0];

      uint16_t *in_base = (uint16_t *)(indata + ch * width);
      uint16_t *wih_base = (uint16_t *)(w_ih + h * width);
      for (std::size_t w = 0; w < width/8*8; w += 8) {
        v8u16 v_in_16, v_wih_16[4];
        v4f32 v_in_f[2], v_wih_f[4][2];

        v_in_16 = __msa_ld_b(in_base + w, 0);
        v_wih_16[0] = __msa_ld_h (wih_base + w, 0);
        v_wih_16[1] = __msa_ld_h (wih_base + 1 * width + w, 0);
        v_wih_16[2] = __msa_ld_h (wih_base + 2 * width + w, 0);
        v_wih_16[3] = __msa_ld_h (wih_base + 3 * width + w, 0);

        v_in_f[0]= __msa_fexupr_w(v_in_16);
        v_in_f[1]= __msa_fexupl_w(v_in_16);

        v_wih_f[0][0] = __msa_fexupr_w(v_wih_16[0]);
        v_wih_f[1][0] = __msa_fexupr_w(v_wih_16[1]);
        v_wih_f[2][0] = __msa_fexupr_w(v_wih_16[2]);
        v_wih_f[3][0] = __msa_fexupr_w(v_wih_16[3]);
        v_wih_f[0][1] = __msa_fexupl_w(v_wih_16[0]);
        v_wih_f[1][1] = __msa_fexupl_w(v_wih_16[1]);
        v_wih_f[2][1] = __msa_fexupl_w(v_wih_16[2]);
        v_wih_f[3][1] = __msa_fexupl_w(v_wih_16[3]);

#undef ALTER_FMADD_4
#define ALTER_FMADD_4(_a, _b, _c, ia0, ib0, ic0, ic1) \
        do{                                                             \
          _a[ia0+0] = __msa_fmadd_w(_b[ib0], _c[ic0+0][ic1], _a[ia0+0]); \
          _a[ia0+1] = __msa_fmadd_w(_b[ib0], _c[ic0+1][ic1], _a[ia0+1]); \
          _a[ia0+2] = __msa_fmadd_w(_b[ib0], _c[ic0+2][ic1], _a[ia0+2]); \
          _a[ia0+3] = __msa_fmadd_w(_b[ib0], _c[ic0+3][ic1], _a[ia0+3]); \
        }while(0)

        ALTER_FMADD_4(vsum, v_in_f, v_wih_f, 0, 0, 0,0);
        ALTER_FMADD_4(vsum, v_in_f, v_wih_f, 0, 1, 0,1);
      }

      v4f32 result[4];
      v4f32 v_bih, v_bhh ;
      v_bih =(v4f32) __msa_ld_w(b_ih + h, 0);
      v_bhh =(v4f32) __msa_ld_w(b_hh + h, 0);
      v_bih = v_bih + v_bhh;
      pack_and_add(vsum[0], vsum[1], vsum[2], vsum[3], v_bih, result[0]);
      __msa_st_w((v4i32)result[0], sum_p + ch * 4 * szHidden + h, 0);
    } // end e unalign
  }


  for (std::size_t ch = s; ch < e; ++ch) {
    float *input_gate_p = sum_p + ch * 4 * szHidden;
    float *forget_gate_p = input_gate_p + szHidden;
    float *cell_gate_p = forget_gate_p +  szHidden;
    float *out_gate_p = cell_gate_p +  szHidden;
    float *base =input_gate_p;


    for(std::size_t h = 0;h < szHidden*4/4*4; h += 4 ) {
      v4f32 vhsum[4][2];
      vhsum[0][0] =(v4f32) __msa_ldi_w(0);
      vhsum[0][1] =(v4f32) __msa_ldi_w(0);
      vhsum[1][0] =(v4f32) __msa_ldi_w(0);
      vhsum[1][1] =(v4f32) __msa_ldi_w(0);
      vhsum[2][0] =(v4f32) __msa_ldi_w(0);
      vhsum[2][1] =(v4f32) __msa_ldi_w(0);
      vhsum[3][0] =(v4f32) __msa_ldi_w(0);
      vhsum[3][1] =(v4f32) __msa_ldi_w(0);


      // start = getSystemTime();
      for(std::size_t hh = 0;hh < szHidden/8*8;hh+=8){
        v8u16 v_hin_16, v_whh_16[4];
        v4f32 v_hin_f[2], v_whh_f[4][2];

        v_hin_16 = __msa_ld_h(hx_p + hh, 0);
        v_hin_f[0] = __msa_fexupr_w(v_hin_16);
        v_hin_f[1] = __msa_fexupl_w(v_hin_16);

        v_whh_16[0] = __msa_ld_h((void*)(w_hh + h * szHidden + hh), 0);
        v_whh_16[1] = __msa_ld_h((void*)(w_hh + (h+1) * szHidden + hh), 0);
        v_whh_16[2] = __msa_ld_h((void*)(w_hh + (h+2) * szHidden + hh), 0);
        v_whh_16[3] = __msa_ld_h((void*)(w_hh + (h+3) * szHidden + hh), 0);

        v_whh_f[0][0] = __msa_fexupr_w(v_whh_16[0]);
        v_whh_f[0][1] = __msa_fexupl_w(v_whh_16[0]);
        v_whh_f[1][0] = __msa_fexupr_w(v_whh_16[1]);
        v_whh_f[1][1] = __msa_fexupl_w(v_whh_16[1]);
        v_whh_f[2][0] = __msa_fexupr_w(v_whh_16[2]);
        v_whh_f[2][1] = __msa_fexupl_w(v_whh_16[2]);
        v_whh_f[3][0] = __msa_fexupr_w(v_whh_16[3]);
        v_whh_f[3][1] = __msa_fexupl_w(v_whh_16[3]);

        vhsum[0][0] = __msa_fmadd_w(v_hin_f[0], v_whh_f[0][0], vhsum[0][0]);
        vhsum[0][1] = __msa_fmadd_w(v_hin_f[1], v_whh_f[0][1], vhsum[0][1]);

        vhsum[1][0] = __msa_fmadd_w(v_hin_f[0], v_whh_f[1][0], vhsum[1][0]);
        vhsum[1][1] = __msa_fmadd_w(v_hin_f[1], v_whh_f[1][1], vhsum[1][1]);

        vhsum[2][0] = __msa_fmadd_w(v_hin_f[0], v_whh_f[2][0], vhsum[2][0]);
        vhsum[2][1] = __msa_fmadd_w(v_hin_f[1], v_whh_f[2][1], vhsum[2][1]);

        vhsum[3][0] = __msa_fmadd_w(v_hin_f[0], v_whh_f[3][0], vhsum[3][0]);
        vhsum[3][1] = __msa_fmadd_w(v_hin_f[1], v_whh_f[3][1], vhsum[3][1]);
      }
      v4f32 result[4];
      result[0] = vhsum[0][0] + vhsum[0][1];
      result[1] = vhsum[1][0] + vhsum[1][1];
      result[2] = vhsum[2][0] + vhsum[2][1];
      result[3] = vhsum[3][0] + vhsum[3][1];
      v4f32 vbase =(v4f32) __msa_ld_w(base + h, 0);
      pack_and_add(result[0], result[1],result[2],result[3], vbase, vbase);
      __msa_st_w((v4i32)vbase, base + h, 0);
    }

#if 1
    for(std::size_t h = 0;h < szHidden/4*4;h+=4){
      v4f32 i_gate, f_gate, c_gate, o_gate;
      v4f32 i_gate_nexp, f_gate_nexp, c_gate_nexp, c_gate_exp, o_gate_nexp;
      i_gate =(v4f32) __msa_ld_w(input_gate_p + h, 0);
      f_gate =(v4f32) __msa_ld_w(forget_gate_p + h, 0);
      c_gate =(v4f32) __msa_ld_w(cell_gate_p + h, 0);
      o_gate =(v4f32) __msa_ld_w(out_gate_p + h, 0);
      get_neg_exp(i_gate, i_gate_nexp);
      get_neg_exp(f_gate, f_gate_nexp);
      get_neg_exp(o_gate, o_gate_nexp);
      get_sigmod(i_gate_nexp, i_gate);
      get_sigmod(f_gate_nexp, f_gate);
      get_sigmod(o_gate_nexp, o_gate);
      get_tan(c_gate, c_gate);

      float cy,hy;
      v4f32 vcy, vcx, vhy, vcy_tan;
      vcx =(v4f32) __msa_ld_w(cx_p + h,0);
      vcy = f_gate * vcx + i_gate * c_gate;
      get_tan(vcy, vcy_tan);
      vhy = o_gate * vcy_tan;

      __msa_st_w((v4i32)vcy, cx_p + h, 0);
      v8u16 vhy_16 = __msa_fexdo_h(vhy, vhy);
      out_p[ch * szHidden * 2 + h] = vhy_16[0];
      out_p[ch * szHidden * 2 + h+1] = vhy_16[1];
      out_p[ch * szHidden * 2 + h+2] = vhy_16[2];
      out_p[ch * szHidden * 2 + h+3] = vhy_16[3];
    }
#else // step3_mmsa
    for(std::size_t h = 0;h < szHidden;h++){

      input_gate = sigmoid(input_gate_p[h]);
      forget_gate = sigmoid(forget_gate_p[h]);
      cell_gate  = std::tanh(cell_gate_p[h]);   //   (e^x - e^(-x)) / (e^x + e^(-x))
      out_gate   = sigmoid(out_gate_p[h]);

      float cy,hy;
      cy = forget_gate * cx_p[h] + input_gate * cell_gate;
      hy = out_gate * std::tanh(cy);
      cx_p[h] = cy;
      out_p[ch * szHidden * 2 + h] = cvt32fto16f(hy);
    }
#endif // step3

    for(std::size_t i = 0;i < szHidden;i++){
      hx_p[i] = out_p[ch * szHidden * 2 + i];
    }

  }
  // printf("[lstm_t ] step1 %7lld, step2 %7lld, step3 %7lld \n", step1, step2, step3);
}

void BidirectionalLSTM_f16_msa::single_lstm_bwd(const tensor_uint16 &in, tensor_uint16 &out,const std::shared_ptr<Parameters> &p,std::size_t off,std::size_t s, std::size_t e) const
{

  std::size_t batch = 0;
  std::size_t chan_size = in.shape(1), width = in.shape(2);
  assert(width%8 == 0);
  std::size_t ig_off = 0 * szHidden;
  std::size_t fg_off = 1 * szHidden;
  std::size_t cell_off = 2 * szHidden;
  std::size_t out_off = 3 * szHidden;
  float input_gate = 0,forget_gate = 0,cell_gate = 0,out_gate = 0;

  tensor_int16 hx{szHidden};
  tensor_float cx{szHidden};
  tensor_float sum_tmp{e, szHidden*4};

  memset(hx.get_raw(),0,hx.size() * sizeof(short));
  memset(cx.get_raw(),0,cx.size() * sizeof(float));

  const uint16_t *w_ih = p->w_ih.get_raw();
  const uint16_t *w_hh = p->w_hh.get_raw();
  const uint16_t *indata = in.get_raw();
  long long start, step1 = 0, step2 = 0, finish_1, finish_2, step3 = 0;
  float* b_hh =  (float*)p->b_hh.get_raw();
  float* b_ih =  (float*)p->b_ih.get_raw();
  float *cx_p = cx.get_raw();
  short *hx_p = hx.get_raw();
  float *sum_p = sum_tmp.get_raw();
  uint16_t *out_p = out.get_raw();


  for(std::size_t h = 0;h < szHidden * 4/4*4 ; h += 4){
    std::size_t ch;
    for (ch = s; ch < e/4*4; ch +=4) {
      v4f32 vsum[4][4];
      vsum[0][0] = (v4f32)__msa_ldi_w(0);
      vsum[1][0] = vsum[0][0];
      vsum[2][0] = vsum[0][0];
      vsum[3][0] = vsum[0][0];

      vsum[0][1] = vsum[0][0]; vsum[0][2] = vsum[0][0]; vsum[0][3] = vsum[0][0];
      vsum[1][1] = vsum[0][0]; vsum[1][2] = vsum[0][0]; vsum[1][3] = vsum[0][0];
      vsum[2][1] = vsum[0][0]; vsum[2][2] = vsum[0][0]; vsum[2][3] = vsum[0][0];
      vsum[3][1] = vsum[0][0]; vsum[3][2] = vsum[0][0]; vsum[3][3] = vsum[0][0];

      uint16_t *wih_base = (uint16_t *)(w_ih + h * width);
      uint16_t *in_base_rev = (uint16_t *)(indata + (e - 1 - ch) * width);

      for (std::size_t w = 0; w < width/8*8; w += 8) {
        v8u16 v_in_16[4], v_wih_16[4];
        v4f32 v_in_f[4][2], v_wih_f[4][2];

        v_in_16[3] = __msa_ld_b(in_base_rev - 3 * width + w, 0);
        v_in_16[2] = __msa_ld_b(in_base_rev - 2 * width + w, 0);
        v_in_16[1] = __msa_ld_b(in_base_rev - 1 * width + w, 0);
        v_in_16[0] = __msa_ld_b(in_base_rev + w, 0);

        v_wih_16[0] = __msa_ld_h (wih_base + w, 0);
        v_wih_16[1] = __msa_ld_h (wih_base + 1 * width + w, 0);
        v_wih_16[2] = __msa_ld_h (wih_base + 2 * width + w, 0);
        v_wih_16[3] = __msa_ld_h (wih_base + 3 * width + w, 0);

        v_wih_f[0][0] = __msa_fexupr_w(v_wih_16[0]);
        v_wih_f[1][0] = __msa_fexupr_w(v_wih_16[1]);
        v_wih_f[2][0] = __msa_fexupr_w(v_wih_16[2]);
        v_wih_f[3][0] = __msa_fexupr_w(v_wih_16[3]);
        v_wih_f[0][1] = __msa_fexupl_w(v_wih_16[0]);
        v_wih_f[1][1] = __msa_fexupl_w(v_wih_16[1]);
        v_wih_f[2][1] = __msa_fexupl_w(v_wih_16[2]);
        v_wih_f[3][1] = __msa_fexupl_w(v_wih_16[3]);

        v_in_f[0][0]= __msa_fexupr_w(v_in_16[0]);
        v_in_f[1][0]= __msa_fexupr_w(v_in_16[1]);
        v_in_f[2][0]= __msa_fexupr_w(v_in_16[2]);
        v_in_f[3][0]= __msa_fexupr_w(v_in_16[3]);

#undef ALTER_FMADD_4
#define ALTER_FMADD_4(_a, _b, _c, ia0, ia1, ib0, ib1, ic0, ic1) \
        do{                                                             \
          _a[ia0+0][ia1] = __msa_fmadd_w(_b[ib0+0][ib1], _c[ic0][ic1], _a[ia0+0][ia1]); \
          _a[ia0+1][ia1] = __msa_fmadd_w(_b[ib0+1][ib1], _c[ic0][ic1], _a[ia0+1][ia1]); \
          _a[ia0+2][ia1] = __msa_fmadd_w(_b[ib0+2][ib1], _c[ic0][ic1], _a[ia0+2][ia1]); \
          _a[ia0+3][ia1] = __msa_fmadd_w(_b[ib0+3][ib1], _c[ic0][ic1], _a[ia0+3][ia1]); \
        }while(0)
        ALTER_FMADD_4(vsum, v_in_f, v_wih_f, 0,0, 0,0, 0,0);
        ALTER_FMADD_4(vsum, v_in_f, v_wih_f, 0,1, 0,0, 1,0);
        ALTER_FMADD_4(vsum, v_in_f, v_wih_f, 0,2, 0,0, 2,0);
        ALTER_FMADD_4(vsum, v_in_f, v_wih_f, 0,3, 0,0, 3,0);

        v_in_f[0][1]= __msa_fexupl_w(v_in_16[0]);
        v_in_f[1][1]= __msa_fexupl_w(v_in_16[1]);
        v_in_f[2][1]= __msa_fexupl_w(v_in_16[2]);
        v_in_f[3][1]= __msa_fexupl_w(v_in_16[3]);

        ALTER_FMADD_4(vsum, v_in_f, v_wih_f, 0,0, 0,1, 0,1);
        ALTER_FMADD_4(vsum, v_in_f, v_wih_f, 0,1, 0,1, 1,1);
        ALTER_FMADD_4(vsum, v_in_f, v_wih_f, 0,2, 0,1, 2,1);
        ALTER_FMADD_4(vsum, v_in_f, v_wih_f, 0,3, 0,1, 3,1);

      }
      v4f32 result[4];
      v4f32 v_bih, v_bhh ;
      v_bih =(v4f32) __msa_ld_w(b_ih + h, 0);
      v_bhh =(v4f32) __msa_ld_w(b_hh + h, 0);
      v_bih = v_bih + v_bhh;
      pack_and_add(vsum[0][0], vsum[0][1], vsum[0][2], vsum[0][3], v_bih, result[0]);
      pack_and_add(vsum[1][0], vsum[1][1], vsum[1][2], vsum[1][3], v_bih, result[1]);
      pack_and_add(vsum[2][0], vsum[2][1], vsum[2][2], vsum[2][3], v_bih, result[2]);
      pack_and_add(vsum[3][0], vsum[3][1], vsum[3][2], vsum[3][3], v_bih, result[3]);
      __msa_st_w((v4i32)result[0], sum_p + ch * 4 * szHidden + h, 0);
      __msa_st_w((v4i32)result[1], sum_p + (ch+1) * 4 * szHidden + h, 0);
      __msa_st_w((v4i32)result[2], sum_p + (ch+2) * 4 * szHidden + h, 0);
      __msa_st_w((v4i32)result[3], sum_p + (ch+3) * 4 * szHidden + h, 0);
    }

        // for e unalign
    for (; ch < e; ch +=1) {
      v4f32 vsum[4];
      vsum[0] = (v4f32)__msa_ldi_w(0);
      vsum[1] = vsum[0];
      vsum[2] = vsum[0];
      vsum[3] = vsum[0];

      uint16_t *wih_base = (uint16_t *)(w_ih + h * width);
      uint16_t *in_base_rev = (uint16_t *)(indata + (e - 1 - ch) * width);
      for (std::size_t w = 0; w < width/8*8; w += 8) {
        v8u16 v_in_16, v_wih_16[4];
        v4f32 v_in_f[2], v_wih_f[4][2];

        v_in_16 = __msa_ld_b(in_base_rev + w, 0);
        v_wih_16[0] = __msa_ld_h (wih_base + w, 0);
        v_wih_16[1] = __msa_ld_h (wih_base + 1 * width + w, 0);
        v_wih_16[2] = __msa_ld_h (wih_base + 2 * width + w, 0);
        v_wih_16[3] = __msa_ld_h (wih_base + 3 * width + w, 0);

        v_in_f[0]= __msa_fexupr_w(v_in_16);
        v_in_f[1]= __msa_fexupl_w(v_in_16);

        v_wih_f[0][0] = __msa_fexupr_w(v_wih_16[0]);
        v_wih_f[1][0] = __msa_fexupr_w(v_wih_16[1]);
        v_wih_f[2][0] = __msa_fexupr_w(v_wih_16[2]);
        v_wih_f[3][0] = __msa_fexupr_w(v_wih_16[3]);
        v_wih_f[0][1] = __msa_fexupl_w(v_wih_16[0]);
        v_wih_f[1][1] = __msa_fexupl_w(v_wih_16[1]);
        v_wih_f[2][1] = __msa_fexupl_w(v_wih_16[2]);
        v_wih_f[3][1] = __msa_fexupl_w(v_wih_16[3]);

#undef ALTER_FMADD_4
#define ALTER_FMADD_4(_a, _b, _c, ia0, ib0, ic0, ic1) \
        do{                                                             \
          _a[ia0+0] = __msa_fmadd_w(_b[ib0], _c[ic0+0][ic1], _a[ia0+0]); \
          _a[ia0+1] = __msa_fmadd_w(_b[ib0], _c[ic0+1][ic1], _a[ia0+1]); \
          _a[ia0+2] = __msa_fmadd_w(_b[ib0], _c[ic0+2][ic1], _a[ia0+2]); \
          _a[ia0+3] = __msa_fmadd_w(_b[ib0], _c[ic0+3][ic1], _a[ia0+3]); \
        }while(0)

        ALTER_FMADD_4(vsum, v_in_f, v_wih_f, 0, 0, 0,0);
        ALTER_FMADD_4(vsum, v_in_f, v_wih_f, 0, 1, 0,1);
      }

      v4f32 result[4];
      v4f32 v_bih, v_bhh ;
      v_bih =(v4f32) __msa_ld_w(b_ih + h, 0);
      v_bhh =(v4f32) __msa_ld_w(b_hh + h, 0);
      v_bih = v_bih + v_bhh;
      pack_and_add(vsum[0], vsum[1], vsum[2], vsum[3], v_bih, result[0]);
      __msa_st_w((v4i32)result[0], sum_p + ch * 4 * szHidden + h, 0);
    } // end e unalign
  }

  for (std::size_t ch = s; ch < e; ++ch) {
    float *input_gate_p = sum_p + ch * 4 * szHidden;
    float *forget_gate_p = input_gate_p + szHidden;
    float *cell_gate_p = forget_gate_p +  szHidden;
    float *out_gate_p = cell_gate_p +  szHidden;
    float *base =input_gate_p;

    for(std::size_t h = 0;h < szHidden*4/4*4; h += 4 ) {
      v4f32 vhsum[4][2];
      vhsum[0][0] =(v4f32) __msa_ldi_w(0);
      vhsum[0][1] =(v4f32) __msa_ldi_w(0);
      vhsum[1][0] =(v4f32) __msa_ldi_w(0);
      vhsum[1][1] =(v4f32) __msa_ldi_w(0);
      vhsum[2][0] =(v4f32) __msa_ldi_w(0);
      vhsum[2][1] =(v4f32) __msa_ldi_w(0);
      vhsum[3][0] =(v4f32) __msa_ldi_w(0);
      vhsum[3][1] =(v4f32) __msa_ldi_w(0);



      for(std::size_t hh = 0;hh < szHidden/8*8;hh+=8){
        v8u16 v_hin_16, v_whh_16[4];
        v4f32 v_hin_f[2], v_whh_f[4][2];

        v_hin_16 = __msa_ld_h(hx_p + hh, 0);
        v_hin_f[0] = __msa_fexupr_w(v_hin_16);
        v_hin_f[1] = __msa_fexupl_w(v_hin_16);

        v_whh_16[0] = __msa_ld_h((void*)(w_hh + h * szHidden + hh), 0);
        v_whh_16[1] = __msa_ld_h((void*)(w_hh + (h+1) * szHidden + hh), 0);
        v_whh_16[2] = __msa_ld_h((void*)(w_hh + (h+2) * szHidden + hh), 0);
        v_whh_16[3] = __msa_ld_h((void*)(w_hh + (h+3) * szHidden + hh), 0);

        v_whh_f[0][0] = __msa_fexupr_w(v_whh_16[0]);
        v_whh_f[0][1] = __msa_fexupl_w(v_whh_16[0]);
        v_whh_f[1][0] = __msa_fexupr_w(v_whh_16[1]);
        v_whh_f[1][1] = __msa_fexupl_w(v_whh_16[1]);
        v_whh_f[2][0] = __msa_fexupr_w(v_whh_16[2]);
        v_whh_f[2][1] = __msa_fexupl_w(v_whh_16[2]);
        v_whh_f[3][0] = __msa_fexupr_w(v_whh_16[3]);
        v_whh_f[3][1] = __msa_fexupl_w(v_whh_16[3]);

        vhsum[0][0] = __msa_fmadd_w(v_hin_f[0], v_whh_f[0][0], vhsum[0][0]);
        vhsum[0][1] = __msa_fmadd_w(v_hin_f[1], v_whh_f[0][1], vhsum[0][1]);

        vhsum[1][0] = __msa_fmadd_w(v_hin_f[0], v_whh_f[1][0], vhsum[1][0]);
        vhsum[1][1] = __msa_fmadd_w(v_hin_f[1], v_whh_f[1][1], vhsum[1][1]);

        vhsum[2][0] = __msa_fmadd_w(v_hin_f[0], v_whh_f[2][0], vhsum[2][0]);
        vhsum[2][1] = __msa_fmadd_w(v_hin_f[1], v_whh_f[2][1], vhsum[2][1]);

        vhsum[3][0] = __msa_fmadd_w(v_hin_f[0], v_whh_f[3][0], vhsum[3][0]);
        vhsum[3][1] = __msa_fmadd_w(v_hin_f[1], v_whh_f[3][1], vhsum[3][1]);
      }
      v4f32 result[4];
      result[0] = vhsum[0][0] + vhsum[0][1];
      result[1] = vhsum[1][0] + vhsum[1][1];
      result[2] = vhsum[2][0] + vhsum[2][1];
      result[3] = vhsum[3][0] + vhsum[3][1];

      v4f32 vbase =(v4f32) __msa_ld_w(base + h, 0);
      pack_and_add(result[0], result[1],result[2],result[3], vbase, vbase);
      __msa_st_w((v4i32)vbase, base + h, 0);
    }

#if 1
    for(std::size_t h = 0;h < szHidden/4*4;h+=4){
      finish_1 = getSystemTime();
      v4f32 i_gate, f_gate, c_gate, o_gate;
      v4f32 i_gate_nexp, f_gate_nexp, c_gate_nexp, c_gate_exp, o_gate_nexp;
      i_gate =(v4f32) __msa_ld_w(input_gate_p + h, 0);
      f_gate =(v4f32) __msa_ld_w(forget_gate_p + h, 0);
      c_gate =(v4f32) __msa_ld_w(cell_gate_p + h, 0);
      o_gate =(v4f32) __msa_ld_w(out_gate_p + h, 0);
      get_neg_exp(i_gate, i_gate_nexp);
      get_neg_exp(f_gate, f_gate_nexp);
      get_neg_exp(o_gate, o_gate_nexp);
      get_sigmod(i_gate_nexp, i_gate);
      get_sigmod(f_gate_nexp, f_gate);
      get_sigmod(o_gate_nexp, o_gate);
      get_tan(c_gate, c_gate);

      float cy,hy;
      v4f32 vcy, vcx, vhy, vcy_tan;
      vcx =(v4f32) __msa_ld_w(cx_p + h,0);
      vcy = f_gate * vcx + i_gate * c_gate;
      get_tan(vcy, vcy_tan);
      vhy = o_gate * vcy_tan;


      __msa_st_w((v4i32)vcy, cx_p + h, 0);
      v8u16 vhy_16 = __msa_fexdo_h(vhy, vhy);
      out_p[(e - 1 - ch) * szHidden * 2 + + off + h] = vhy_16[0];
      out_p[(e - 1 - ch) * szHidden * 2 + + off + h+1] = vhy_16[1];
      out_p[(e - 1 - ch) * szHidden * 2 + + off + h+2] = vhy_16[2];
      out_p[(e - 1 - ch) * szHidden * 2 + + off + h+3] = vhy_16[3];

    }
#else // step3_mmsa

    for(std::size_t h = 0;h < szHidden;h++){
      input_gate = sigmoid(input_gate_p[h]);
      forget_gate = sigmoid(forget_gate_p[h]);
      cell_gate  = std::tanh(cell_gate_p[h]);   //   (e^x - e^(-x)) / (e^x + e^(-x))
      out_gate   = sigmoid(out_gate_p[h]);
      float cy,hy;
      cy = forget_gate * cx_p[h] + input_gate * cell_gate;
      hy = out_gate * std::tanh(cy);
      cx_p[h] = cy;
      out_p[(e - 1 - ch) * szHidden * 2 + off + h] = cvt32fto16f(hy);
    }
#endif
    for(std::size_t i = 0;i < szHidden;i++){
      hx_p[i] = out_p[(e - 1 - ch) * szHidden * 2 + off + i];
    }
  }
  // printf("[lstm_t ] step1 %7lld, step2 %7lld, step3 %7lld \n", step1, step2, step3);

}

}

#else
#error unsupport MIPS MSA compile for MSA optimized LSTM.
#endif
