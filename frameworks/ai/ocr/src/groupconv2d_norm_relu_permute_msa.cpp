#include <cstring>
#include <type_traits>

#include "layers/layer.h"
#include "layers/groupconv2d_norm_relu_permute_msa.h"
#include <math.h>
#if defined(__mips_msa)
#include <msa.h>

namespace tnn {

tensorX groupconv2d_norm_relu_permute_msa::forward(tensorX &&tensor, thread_pool &threads,std::shared_ptr<TensorParams> param) const {
   tensor_float x = tensor.getTensor();
   assert(x.ndim() == 4 && x.shape(3) == m_in_channels);
   std::size_t n = x.shape(0), start = 0;
   std::vector<std::future<void> > sync;
   double step;

   if (m_padding) {
     sync.reserve(threads.get_thread_num());
     tensor_float temp{n, x.shape(1) + 2 * m_padding, x.shape(2) + 2 * m_padding, x.shape(3)};
     step = (double) n * x.shape(1) * x.shape(2) / threads.get_thread_num();
     for (std::size_t i = 0; i < threads.get_thread_num(); ++i) {
       std::size_t end = (int) (step * (i + 1) + 0.5);
       if (start != end)
         sync.emplace_back(threads.enqueue([this, &temp, &x](std::size_t s, std::size_t e) {
               for (std::size_t j = s; j < e; ++j) {
                 std::size_t h = j / x.shape(2), w = j % x.shape(2);
                 memcpy(temp.get_raw(h + m_padding, w + m_padding, 0), x.get_raw(h, w, 0), x.shape(3) * sizeof(float));
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

   single_conv_group(x, y, 0,param);

   return y;

 }

void groupconv2d_norm_relu_permute_msa::load(std::istream &in) {
  tensor_float weight(m_weight.shape());
  weight.load(in);

  m_weight.reshape({m_weight.shape(2), m_weight.shape(3), m_weight.shape(0), m_weight.shape(1)});

  float *p_m_weight = m_weight.get_raw();
  for( size_t g = 0; g < m_out_channels; g++)
    for(size_t  h = 0; h < m_kernel_size; h++)
      for(size_t w = 0; w < m_kernel_size; w++)
      {
        m_weight.at(h,w,g,0) = weight.at(g,0,h,w);
      }
   if (m_has_bias)
     m_bias.load(in);
   m_norm_weight.load(in);
   m_norm_bias.load(in);
   m_norm_mean.load(in);
   m_norm_var.load(in);
   m_norm_batch_num.load(in);

   for (int c = 0; c < m_features; c++) {
     float inv_var = 1 / sqrt(m_norm_var.at(c) + static_cast<float>(m_eps));
     float alpha = inv_var * m_norm_weight.at(c);
     m_beta.at(c) = m_norm_bias.at(c) - m_norm_mean.at(c) * alpha;
     m_alpha.at(c) = alpha;
   }
 }

void groupconv2d_norm_relu_permute_msa::single_conv_group(const tensor_float &x, tensor_float &y, std::size_t i,std::shared_ptr<TensorParams> param) const
{
  assert(m_group % 8 == 0 && m_group==m_in_channels && m_group==m_out_channels);
  std::size_t height = y.shape(1), width = y.shape(2);
  std::size_t in_height = x.shape(1), in_width = x.shape(2);
  const float *p_x = x.get_raw();
  const float *p_m_weight = m_weight.get_raw();
  const float *p_m_bias = m_bias.get_raw();
  const float *p_m_alpha = m_alpha.get_raw();
  const float *p_m_beta = m_beta.get_raw();
  float *p_y = y.get_raw();

  v4f32 zero = (v4f32)__msa_ldi_w(0);
  v4f32 fmax = zero;
  for (std::size_t h = 0; h < height; h++)
  {
    std::size_t hs = m_stride * h;
    std::size_t w = 0;
#if 1
    for (; w < width / 2 * 2; w += 2)
    {
      std::size_t ws = m_stride * w;
      std::size_t g = 0;
      for (; g < m_group / 8 * 8; g += 8)
      {
        v4f32 vsum[2][2];
        vsum[0][0] = (v4f32)__msa_ldi_w(0);
        vsum[0][1] = vsum[0][0];
        vsum[1][0] = vsum[0][0];
        vsum[1][1] = vsum[0][0];
        for (std::size_t kh = 0; kh < m_kernel_size; ++kh)
        {
          float *in_col_p = (float *)p_x + ((i * in_height + hs + kh) * in_width + ws) * m_in_channels + g;
          float *w_col_p = (float *)p_m_weight + kh * m_kernel_size * m_out_channels + g;
          v4f32 v_in[2];
          v_in[0] = (v4f32)__msa_ld_w(in_col_p, 0);
          v_in[1] = (v4f32)__msa_ld_w(in_col_p, 16);
          v4f32 v_in_next[2];
          for (std::size_t kw = 0; kw < m_kernel_size; ++kw)
          {
            v_in_next[0] = (v4f32)__msa_ld_w(in_col_p + (kw + 1) * m_in_channels, 0);
            v_in_next[1] = (v4f32)__msa_ld_w(in_col_p + (kw + 1) * m_in_channels, 16);

            v4f32 v_wght[2];
            v_wght[0] = (v4f32)__msa_ld_w(w_col_p + kw * m_out_channels, 0);
            v_wght[1] = (v4f32)__msa_ld_w(w_col_p + kw * m_out_channels, 16);

            vsum[0][0] = __msa_fmadd_w(v_in[0], v_wght[0], vsum[0][0]);
            vsum[0][1] = __msa_fmadd_w(v_in[1], v_wght[1], vsum[0][1]);

            vsum[1][0] = __msa_fmadd_w(v_in_next[0], v_wght[0], vsum[1][0]);
            vsum[1][1] = __msa_fmadd_w(v_in_next[1], v_wght[1], vsum[1][1]);
            v_in[0] = v_in_next[0];
            v_in[1] = v_in_next[1];
            //sum += p_x[((i * in_height + hs + kh) * in_width + ws + kw) * m_in_channels + g] * p_m_weight[(kh * m_kernel_size + kw) * m_out_channels + g];
          }
        }
        v4f32 vbias[2], valpha[2], vbeta[2];
        vbias[0] = (v4f32)__msa_ld_w((float *)p_m_bias + g, 0);
        vbias[1] = (v4f32)__msa_ld_w((float *)p_m_bias + g, 16);

        valpha[0] = (v4f32)__msa_ld_w((float *)p_m_alpha + g, 0);
        valpha[1] = (v4f32)__msa_ld_w((float *)p_m_alpha + g, 16);

        vbeta[0] = (v4f32)__msa_ld_w((float *)p_m_beta + g, 0);
        vbeta[1] = (v4f32)__msa_ld_w((float *)p_m_beta + g, 16);

        vbeta[0] = (v4f32)__msa_fmadd_w(vbias[0], valpha[0], vbeta[0]);
        vbeta[1] = (v4f32)__msa_fmadd_w(vbias[1], valpha[1], vbeta[1]);

        vsum[0][0] = (v4f32)__msa_fmadd_w(vsum[0][0], valpha[0], vbeta[0]);
        vsum[0][1] = (v4f32)__msa_fmadd_w(vsum[0][1], valpha[1], vbeta[1]);
        vsum[1][0] = (v4f32)__msa_fmadd_w(vsum[1][0], valpha[0], vbeta[0]);
        vsum[1][1] = (v4f32)__msa_fmadd_w(vsum[1][1], valpha[1], vbeta[1]);

        vsum[0][0] = __msa_fmax_w(vsum[0][0], zero);
        vsum[0][1] = __msa_fmax_w(vsum[0][1], zero);
        vsum[1][0] = __msa_fmax_w(vsum[1][0], zero);
        vsum[1][1] = __msa_fmax_w(vsum[1][1], zero);
        float *out_base = p_y + ((i * height + h) * width + w) * m_out_channels + g;
        __msa_st_w((v4i32)vsum[0][0], out_base, 0);
        __msa_st_w((v4i32)vsum[0][1], out_base, 16);
        __msa_st_w((v4i32)vsum[1][0], out_base + m_out_channels, 0);
        __msa_st_w((v4i32)vsum[1][1], out_base + m_out_channels, 16);
        v4f32 vmax[2];
        vmax[0] = __msa_fmax_w(vsum[0][0], vsum[0][1]);
        vmax[1] = __msa_fmax_w(vsum[1][0], vsum[1][1]);
        fmax = __msa_fmax_w(fmax, vmax[0]);
        fmax = __msa_fmax_w(fmax, vmax[1]);
      }
    }
#endif

    for (; w < width; ++w)
    {
      std::size_t ws = m_stride * w;
      std::size_t g = 0;
      for (; g < m_group / 8 * 8; g += 8)
      {
        //float sum = 0;
        v4f32 vsum[2];
        vsum[0] = (v4f32)__msa_ldi_w(0);
        vsum[1] = vsum[0];
        for (std::size_t kh = 0; kh < m_kernel_size; ++kh)
        {
          float *in_col_p = (float *)p_x + ((i * in_height + hs + kh) * in_width + ws) * m_in_channels + g;
          float *w_col_p = (float *)p_m_weight + kh * m_kernel_size * m_out_channels + g;
          for (std::size_t kw = 0; kw < m_kernel_size; ++kw)
          {
            v4f32 v_in[2];
            v_in[0] = (v4f32)__msa_ld_w(in_col_p + kw * m_in_channels, 0);
            v_in[1] = (v4f32)__msa_ld_w(in_col_p + kw * m_in_channels, 16);

            v4f32 v_wght[2];
            v_wght[0] = (v4f32)__msa_ld_w(w_col_p + kw * m_out_channels, 0);
            v_wght[1] = (v4f32)__msa_ld_w(w_col_p + kw * m_out_channels, 16);

            vsum[0] = __msa_fmadd_w(v_in[0], v_wght[0], vsum[0]);
            vsum[1] = __msa_fmadd_w(v_in[1], v_wght[1], vsum[1]);
          }
        }
        v4f32 vbias[2], valpha[2], vbeta[2];
        vbias[0] = (v4f32)__msa_ld_w((float *)p_m_bias + g, 0);
        vbias[1] = (v4f32)__msa_ld_w((float *)p_m_bias + g, 16);

        valpha[0] = (v4f32)__msa_ld_w((float *)p_m_alpha + g, 0);
        valpha[1] = (v4f32)__msa_ld_w((float *)p_m_alpha + g, 16);

        vbeta[0] = (v4f32)__msa_ld_w((float *)p_m_beta + g, 0);
        vbeta[1] = (v4f32)__msa_ld_w((float *)p_m_beta + g, 16);

        vbeta[0] = (v4f32)__msa_fmadd_w(vbias[0], valpha[0], vbeta[0]);
        vbeta[1] = (v4f32)__msa_fmadd_w(vbias[1], valpha[1], vbeta[1]);

        vsum[0] = (v4f32)__msa_fmadd_w(vsum[0], valpha[0], vbeta[0]);
        vsum[1] = (v4f32)__msa_fmadd_w(vsum[1], valpha[1], vbeta[1]);

        vsum[0] = __msa_fmax_w(vsum[0], zero);
        vsum[1] = __msa_fmax_w(vsum[1], zero);
        float *out_base = p_y + ((i * height + h) * width + w) * m_out_channels + g;
        __msa_st_w((v4i32)vsum[0], out_base, 0);
        __msa_st_w((v4i32)vsum[1], out_base, 16);
        v4f32 vmax;
        vmax = __msa_fmax_w(vsum[0], vsum[1]);
        fmax = __msa_fmax_w(fmax, vmax);
      }
    }
  }
  v4f32 pack;
  pack = (v4f32)__msa_shf_w((v4i32)fmax, 0B10110001);
  fmax = __msa_fmax_w(fmax, pack);
  pack = (v4f32)__msa_shf_w((v4i32)fmax, 0B00001010);
  fmax = __msa_fmax_w(fmax, pack);
  param->input_max = fmax[0];
}

}
#else
#error unsupport MIPS MSA compile for MSA optimized groupconv2d_norm_relu_permute.
#endif
