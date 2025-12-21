#include <cstring>
#include <type_traits>
#include <cmath>
#include "layers/layer.h"
#include "layers/conv2d_fixpoint_permute_msa.h"
#include <fstream>
#if defined(__mips_msa)
#include <msa.h>

#define ALTER_EXTEND_ADD_4_2GROUP(_a, _b, _c, ia0, ia1, ib0, ic0)                 \
  do                                                                              \
  {                                                                               \
    v8i16 _vt[4][2];                                                              \
    _vt[0][0] = __msa_dotp_s_h(_b[ib0 + 0], _c[ic0]);                             \
    _vt[1][0] = __msa_dotp_s_h(_b[ib0 + 1], _c[ic0]);                             \
    _vt[2][0] = __msa_dotp_s_h(_b[ib0 + 2], _c[ic0]);                             \
    _vt[3][0] = __msa_dotp_s_h(_b[ib0 + 3], _c[ic0]);                             \
                                                                                  \
    _vt[0][1] = __msa_dotp_s_h(_b[ib0 + 0], _c[ic0 + 1]);                         \
    _vt[1][1] = __msa_dotp_s_h(_b[ib0 + 1], _c[ic0 + 1]);                         \
    _vt[2][1] = __msa_dotp_s_h(_b[ib0 + 2], _c[ic0 + 1]);                         \
    _vt[3][1] = __msa_dotp_s_h(_b[ib0 + 3], _c[ic0 + 1]);                         \
                                                                                  \
    _a[ia0 + 0][ia1] = __msa_dpadd_s_w(_a[ia0 + 0][ia1], _vt[0][0], one);         \
    _a[ia0 + 1][ia1] = __msa_dpadd_s_w(_a[ia0 + 1][ia1], _vt[1][0], one);         \
    _a[ia0 + 2][ia1] = __msa_dpadd_s_w(_a[ia0 + 2][ia1], _vt[2][0], one);         \
    _a[ia0 + 3][ia1] = __msa_dpadd_s_w(_a[ia0 + 3][ia1], _vt[3][0], one);         \
                                                                                  \
    _a[ia0 + 0][ia1 + 1] = __msa_dpadd_s_w(_a[ia0 + 0][ia1 + 1], _vt[0][1], one); \
    _a[ia0 + 1][ia1 + 1] = __msa_dpadd_s_w(_a[ia0 + 1][ia1 + 1], _vt[1][1], one); \
    _a[ia0 + 2][ia1 + 1] = __msa_dpadd_s_w(_a[ia0 + 2][ia1 + 1], _vt[2][1], one); \
    _a[ia0 + 3][ia1 + 1] = __msa_dpadd_s_w(_a[ia0 + 3][ia1 + 1], _vt[3][1], one); \
  } while (0)

#define ALTER_EXTEND_ADD_4_2GROUP_HALF(_a, _b, _c, ia0, ia1, ib0, ic0)                 \
  do                                                                              \
  {                                                                               \
    v8i16 _vt[4][2];                                                              \
    _vt[0][0] = __msa_dotp_s_h(_b[ib0 + 0], _c[ic0]);                             \
    _vt[1][0] = __msa_dotp_s_h(_b[ib0 + 1], _c[ic0]);                             \
    _vt[2][0] = __msa_dotp_s_h(_b[ib0 + 2], _c[ic0]);                             \
    _vt[3][0] = __msa_dotp_s_h(_b[ib0 + 3], _c[ic0]);                             \
                                                                                  \
    _vt[0][1] = __msa_dotp_s_h(_b[ib0 + 0], _c[ic0 + 1]);                         \
    _vt[1][1] = __msa_dotp_s_h(_b[ib0 + 1], _c[ic0 + 1]);                         \
    _vt[2][1] = __msa_dotp_s_h(_b[ib0 + 2], _c[ic0 + 1]);                         \
    _vt[3][1] = __msa_dotp_s_h(_b[ib0 + 3], _c[ic0 + 1]);                         \
                                                                                  \
    _a[ia0 + 0][ia1] = __msa_accsl_w(_a[ia0 + 0][ia1], _vt[0][0]);                \
    _a[ia0 + 1][ia1] = __msa_accsl_w(_a[ia0 + 1][ia1], _vt[1][0]);                \
    _a[ia0 + 2][ia1] = __msa_accsl_w(_a[ia0 + 2][ia1], _vt[2][0]);                \
    _a[ia0 + 3][ia1] = __msa_accsl_w(_a[ia0 + 3][ia1], _vt[3][0]);                \
                                                                                  \
    _a[ia0 + 0][ia1 + 1] = __msa_accsl_w(_a[ia0 + 0][ia1 + 1], _vt[0][1]);        \
    _a[ia0 + 1][ia1 + 1] = __msa_accsl_w(_a[ia0 + 1][ia1 + 1], _vt[1][1]);        \
    _a[ia0 + 2][ia1 + 1] = __msa_accsl_w(_a[ia0 + 2][ia1 + 1], _vt[2][1]);        \
    _a[ia0 + 3][ia1 + 1] = __msa_accsl_w(_a[ia0 + 3][ia1 + 1], _vt[3][1]);        \
  } while (0)

#define pack_and_add_integer(a0, a1, a2, a3, bias, scale, result) \
  do                                                              \
  {                                                               \
    v4i32 pack_0, pack_1, pack_2, pack_3;                         \
                                                                  \
    pack_0 = __msa_pckev_d((v4i32)a1, (v4i32)a0);                 \
    pack_1 = __msa_pckod_d((v4i32)a1, (v4i32)a0);                 \
    pack_2 = __msa_pckev_d((v4i32)a3, (v4i32)a2);                 \
    pack_3 = __msa_pckod_d((v4i32)a3, (v4i32)a2);                 \
    pack_1 = pack_1 + pack_0;                                     \
    pack_3 = pack_3 + pack_2;                                     \
    pack_0 = __msa_pckev_w(pack_3, pack_1);                       \
    pack_1 = __msa_pckod_w(pack_3, pack_1);                       \
                                                                  \
    result = (v4f32)(pack_0 + pack_1);                            \
    result = __msa_ffint_s_w((v4i32)result);                      \
    result = result * scale + bias;                               \
  } while (0)
namespace tnn {

tensorX conv2d_fixpoint_permute_msa::forward(tensorX &&tensor, thread_pool &threads,std::shared_ptr<TensorParams> param) const {
  tensor_float x = tensor.getTensor();
  assert(x.ndim() == 4 && x.shape(3) == m_in_channels);
  std::size_t n = x.shape(0), start = 0;
  std::vector<std::future<void> > sync;
  double step;
  if (m_padding) {
    sync.reserve(threads.get_thread_num());
    tensor_float temp{n, x.shape(1), x.shape(2) + 2 * m_padding, x.shape(3) + 2 * m_padding};
    step = (double) n * x.shape(1) * x.shape(2) / threads.get_thread_num();
    for (std::size_t i = 0; i < threads.get_thread_num(); ++i) {
      std::size_t end = (int) (step * (i + 1) + 0.5);
      if (start != end)
        sync.emplace_back(threads.enqueue([this, &temp, &x](std::size_t s, std::size_t e) {
                                            for (std::size_t j = s; j < e; ++j) {
                                              std::size_t c = j / x.shape(2), h = j % x.shape(2);
                                              memcpy(temp.get_raw(c, h + m_padding, m_padding), x.get_raw(c, h, 0), x.shape(3) * sizeof(float));
                                            }
                                          }, start, end));
      start = end;
    }
    for (std::size_t i = 0; i < sync.size(); ++i)
      sync[i].get();
    sync.clear();
    start = 0;
    x = std::move(temp);
  }

  tensor_float y{n, (x.shape(1) - m_kernel_size) / m_stride + 1,
        (x.shape(2) - m_kernel_size) / m_stride + 1, m_out_channels};
  sync.reserve(threads.get_thread_num());
  step = (double) n * m_out_channels / threads.get_thread_num();

  if (m_kernel_size == 1) {
    tensor_int8 ix(x.shape());
    float fw = 127.0/param->input_max;
    float_to_uint8(x, ix, fw);
    single_conv_1_1(ix, y,param);
  }else{
    assert(0);
  }

  for (std::size_t i = 0; i < sync.size(); ++i)
    sync[i].get();
  return y;
}
void conv2d_fixpoint_permute_msa::load(std::istream &in) {
  tensor_float weight(m_weight.shape());
  tensor_float bias(m_bias.shape());
  weight.load(in);
  bias.load(in);
  factor_w = tensor_float(m_bias.shape());
  for(int out = 0;out < m_out_channels;out++){
    float fmax = 0;
    for (std::size_t in = 0; in < m_in_channels; ++in)
      for (std::size_t kh = 0; kh < m_kernel_size; ++kh)
        for (std::size_t kw = 0; kw < m_kernel_size; ++kw){
          float f = weight.at(out, in, kh, kw);
          f = fabs(f);
          if(fmax < f)
            fmax = f;
        }
    float fw = 127.0 / fmax;
    for (std::size_t in = 0; in < m_in_channels; ++in)
      for (std::size_t kh = 0; kh < m_kernel_size; ++kh)
        for (std::size_t kw = 0; kw < m_kernel_size; ++kw){
          float f = weight.at(out, in, kh, kw);
          m_weight.at(out, in, kh, kw) = f * fw;
        }
    factor_w.at(out) = 1.0 / fw;
  }
  m_bias  = bias;
}
#pragma GCC optimize("-fno-schedule-insns,-fno-schedule-insns2")
void conv2d_fixpoint_permute_msa::single_conv_1_1(const tensor_int8 &ix, tensor_float &y,std::shared_ptr<TensorParams> param) const
{
  assert(m_in_channels % 8 == 0 && m_out_channels % 4 == 0);
  std::size_t height = y.shape(1), width = y.shape(2);
  const int8_t *p_ix = ix.get_raw();
  const int8_t *p_m_weight = m_weight.get_raw();
  const float *p_m_bias = m_bias.get_raw();
  const float *p_factor_w = factor_w.get_raw();
  float *p_y = y.get_raw();
  float fw = param->input_max / 127.0;
  v4f32 v_fw = (v4f32)__msa_fill_w(*(int *)(&fw));
  v8i16 one = __msa_ldi_h(1);
  for (std::size_t out = 0; out < m_out_channels / 4 * 4; out += 4)
  {
    for (std::size_t h = 0; h < height; ++h)
    {
      float *p_y_base = p_y + h * width * m_out_channels + out;
      std::size_t w = 0;
      for (; w < width / 4 * 4; w += 4)
      {
        int8_t *in_base = ((int8_t *)p_ix + (h * width + w) * m_stride * m_in_channels);
        int8_t *w_base = ((int8_t *)p_m_weight + out * m_in_channels);
        v4i32 vsum[4][4];
        vsum[0][0] = (v4i32)__msa_ldi_w(0);
        vsum[1][0] = vsum[0][0];
        vsum[2][0] = vsum[0][0];
        vsum[3][0] = vsum[0][0];

        vsum[0][1] = vsum[0][0]; vsum[0][2] = vsum[0][0];  vsum[0][3] = vsum[0][0];
        vsum[1][1] = vsum[0][0]; vsum[1][2] = vsum[0][0];  vsum[1][3] = vsum[0][0];
        vsum[2][1] = vsum[0][0]; vsum[2][2] = vsum[0][0];  vsum[2][3] = vsum[0][0];
        vsum[3][1] = vsum[0][0]; vsum[3][2] = vsum[0][0];  vsum[3][3] = vsum[0][0];
        std::size_t in = 0;
        for (; in < m_in_channels / 16 * 16; in += 16)
        {
          v16i8 v_in_8[4], v_w_8[4];
          v_in_8[0] = __msa_ld_b(in_base + in, 0);
          v_in_8[1] = __msa_ld_b(in_base + 1 * m_stride * m_in_channels + in, 0);
          v_in_8[2] = __msa_ld_b(in_base + 2 * m_stride * m_in_channels + in, 0);
          v_in_8[3] = __msa_ld_b(in_base + 3 * m_stride * m_in_channels + in, 0);

          v_w_8[0] = __msa_ld_h(w_base + in, 0);
          v_w_8[1] = __msa_ld_h(w_base + 1 * m_in_channels + in, 0);
          v_w_8[2] = __msa_ld_h(w_base + 2 * m_in_channels + in, 0);
          v_w_8[3] = __msa_ld_h(w_base + 3 * m_in_channels + in, 0);

          ALTER_EXTEND_ADD_4_2GROUP(vsum, v_in_8, v_w_8, 0, 0, 0, 0);
          ALTER_EXTEND_ADD_4_2GROUP(vsum, v_in_8, v_w_8, 0, 2, 0, 2);
        }
        if(in <= m_in_channels - 8)
        {
          in -= 8;
          v16i8 v_in_8[4], v_w_8[4];
          v_in_8[0] = __msa_ld_b(in_base + in, 0);
          v_in_8[1] = __msa_ld_b(in_base + 1 * m_stride * m_in_channels + in, 0);
          v_in_8[2] = __msa_ld_b(in_base + 2 * m_stride * m_in_channels + in, 0);
          v_in_8[3] = __msa_ld_b(in_base + 3 * m_stride * m_in_channels + in, 0);

          v_w_8[0] = __msa_ld_h(w_base + in, 0);
          v_w_8[1] = __msa_ld_h(w_base + 1 * m_in_channels + in, 0);
          v_w_8[2] = __msa_ld_h(w_base + 2 * m_in_channels + in, 0);
          v_w_8[3] = __msa_ld_h(w_base + 3 * m_in_channels + in, 0);

          ALTER_EXTEND_ADD_4_2GROUP_HALF(vsum, v_in_8, v_w_8, 0, 0, 0, 0);
          ALTER_EXTEND_ADD_4_2GROUP_HALF(vsum, v_in_8, v_w_8, 0, 2, 0, 2);
          in += 16;
        }
        v4f32 v_bias, v_factor_w, v_fwfactorw_mul;
        v_factor_w = (v4f32)__msa_ld_w((float *)p_factor_w + out, 0);
        v_bias = (v4f32)__msa_ld_w((float *)p_m_bias + out, 0);
        v_fwfactorw_mul = (v4f32)__msa_fmul_w(v_fw, v_factor_w);
        v4f32 result[4];
        pack_and_add_integer(vsum[0][0], vsum[0][1], vsum[0][2], vsum[0][3], v_bias, v_fwfactorw_mul, result[0]);
        pack_and_add_integer(vsum[1][0], vsum[1][1], vsum[1][2], vsum[1][3], v_bias, v_fwfactorw_mul, result[1]);
        pack_and_add_integer(vsum[2][0], vsum[2][1], vsum[2][2], vsum[2][3], v_bias, v_fwfactorw_mul, result[2]);
        pack_and_add_integer(vsum[3][0], vsum[3][1], vsum[3][2], vsum[3][3], v_bias, v_fwfactorw_mul, result[3]);

        __msa_st_w((v4i32)result[0], p_y_base + w * m_out_channels, 0);
        __msa_st_w((v4i32)result[1], p_y_base + (w + 1) * m_out_channels, 0);
        __msa_st_w((v4i32)result[2], p_y_base + (w + 2) * m_out_channels, 0);
        __msa_st_w((v4i32)result[3], p_y_base + (w + 3) * m_out_channels, 0);
      }
      for (; w < width; w++)
      {
        int8_t *in_base = ((int8_t *)p_ix + (h * width + w) * m_stride * m_in_channels);
        int8_t *w_base = ((int8_t *)p_m_weight + out * m_in_channels);
        v4i32 vsum[4];
        vsum[0] = (v4i32)__msa_ldi_w(0);
        vsum[1] = vsum[0];
        vsum[2] = vsum[0];
        vsum[3] = vsum[0];
        std::size_t in = 0;
        for (; in < m_in_channels / 16 * 16; in += 16)
        {
          v16i8 v_in_8, v_w_8[4];

          v_in_8 = __msa_ld_b(in_base + in, 0);
          v_w_8[0] = __msa_ld_h(w_base + in, 0);
          v_w_8[1] = __msa_ld_h(w_base + 1 * m_in_channels + in, 0);
          v_w_8[2] = __msa_ld_h(w_base + 2 * m_in_channels + in, 0);
          v_w_8[3] = __msa_ld_h(w_base + 3 * m_in_channels + in, 0);

          v8i16 vt[4];
          vt[0] = __msa_dotp_s_h(v_in_8, v_w_8[0]);
          vt[1] = __msa_dotp_s_h(v_in_8, v_w_8[1]);
          vt[2] = __msa_dotp_s_h(v_in_8, v_w_8[2]);
          vt[3] = __msa_dotp_s_h(v_in_8, v_w_8[3]);

          vsum[0] = __msa_dpadd_s_w(vsum[0], vt[0], one);
          vsum[1] = __msa_dpadd_s_w(vsum[1], vt[1], one);
          vsum[2] = __msa_dpadd_s_w(vsum[2], vt[2], one);
          vsum[3] = __msa_dpadd_s_w(vsum[3], vt[3], one);
        }
        if(in <= m_in_channels - 8)
        {
          in -= 8;
          v16i8 v_in_8, v_w_8[4];
          v_in_8 = __msa_ld_b(in_base + in, 0);

          v_w_8[0] = __msa_ld_h(w_base + in, 0);
          v_w_8[1] = __msa_ld_h(w_base + 1 * m_in_channels + in, 0);
          v_w_8[2] = __msa_ld_h(w_base + 2 * m_in_channels + in, 0);
          v_w_8[3] = __msa_ld_h(w_base + 3 * m_in_channels + in, 0);

          v8i16 vt[4];
          vt[0] = __msa_dotp_s_h(v_in_8, v_w_8[0]);
          vt[1] = __msa_dotp_s_h(v_in_8, v_w_8[1]);
          vt[2] = __msa_dotp_s_h(v_in_8, v_w_8[2]);
          vt[3] = __msa_dotp_s_h(v_in_8, v_w_8[3]);

          vsum[0] = __msa_accsl_w(vsum[0], vt[0]);
          vsum[1] = __msa_accsl_w(vsum[1], vt[1]);
          vsum[2] = __msa_accsl_w(vsum[2], vt[2]);
          vsum[3] = __msa_accsl_w(vsum[3], vt[3]);
          in += 16;
        }
        v4f32 v_bias, v_factor_w, v_fwfactorw_mul, result;
        v_factor_w = (v4f32)__msa_ld_w((float *)p_factor_w + out, 0);
        v_bias = (v4f32)__msa_ld_w((float *)p_m_bias + out, 0);
        v_fwfactorw_mul = (v4f32)__msa_fmul_w(v_fw, v_factor_w);
        pack_and_add_integer(vsum[0], vsum[1], vsum[2], vsum[3], v_bias, v_fwfactorw_mul, result);
        __msa_st_w((v4i32)result, p_y_base + w * m_out_channels, 0);
      }
    }
  }
}

void conv2d_fixpoint_permute_msa::single_conv(const tensor_float &x, tensor_float &y) const {
  std::size_t height = y.shape(2), width = y.shape(3);
  tensor_int8 ix(x.shape());
  float fmax = 0;
  for(std::size_t i = 0;i < x.size();i++){
    float f = x.at(i);
    f = fabs(f);
    if(f > fmax)
      fmax = f;
  }
  float fw = 127.0 / fmax;
  for(std::size_t i = 0;i < x.size();i++){
    ix.at(i) = x.at(i) * fw;
  }
  for(std::size_t out = 0;out < m_out_channels;out++){
    for (std::size_t h = 0; h < height; ++h) {
      std::size_t hs = m_stride * h;
      for (std::size_t w = 0; w < width; ++w) {
        int sum = 0;
        std::size_t ws = m_stride * w;

        for (std::size_t in = 0; in < m_in_channels; ++in)
          for (std::size_t kh = 0; kh < m_kernel_size; ++kh)
            for (std::size_t kw = 0; kw < m_kernel_size; ++kw)
              sum += ix.at(0, in, hs + kh, ws + kw) * m_weight.at(out, in, kh, kw);
        y.at(0, out, h, w) =  m_bias.at(out) + sum * factor_w.at(out) * 1.0 / fw;
      }
    }
  }
}

}
#else
#error unsupport MIPS MSA compile for MSA optimized conv2d_fixpoint_permute.
#endif