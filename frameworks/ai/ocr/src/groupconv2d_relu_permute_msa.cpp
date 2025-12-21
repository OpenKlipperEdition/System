#include <cstring>
#include <type_traits>
#include <fstream>

#include "layers/layer.h"
#include "layers/groupconv2d_relu_permute_msa.h"
#if defined(__mips_msa)
#include <msa.h>

namespace tnn
{
  // with input hwc, and output hwc
  tensorX groupconv2d_relu_permute_msa::forward(tensorX &&tensor, thread_pool &threads, std::shared_ptr<TensorParams> param) const
  {
    tensor_float x = tensor.getTensor();
    assert(x.ndim() == 4 && x.shape(3) == m_in_channels);

    std::size_t n = x.shape(0), start = 0;
    std::vector<std::future<void>> sync;
    double step;
    if (m_padding)
    {
      sync.reserve(threads.get_thread_num());
      tensor_float temp{n, x.shape(1) + 2 * m_padding, x.shape(2) + 2 * m_padding, x.shape(3)};

      step = (double)n * x.shape(1) * x.shape(2) / threads.get_thread_num();

      for (std::size_t i = 0; i < threads.get_thread_num(); ++i)
      {
        std::size_t end = (int)(step * (i + 1) + 0.5);
        if (start != end)
          sync.emplace_back(threads.enqueue([this, &temp, &x](std::size_t s, std::size_t e) {
            for (std::size_t j = s; j < e; ++j)
            {
              std::size_t h = j / x.shape(2), w = j % x.shape(2);

              memcpy(temp.get_raw(h + m_padding, w + m_padding, 0), x.get_raw(h, w, 0), x.shape(3) * sizeof(float));
            }
          },
                                            start, end));
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

    single_conv_group(x, y, 0, param);

    return y;
  }
  using std::size_t;

  void groupconv2d_relu_permute_msa::load(std::istream &in)
  {
    tensor_float weight(m_weight.shape());
    weight.load(in);

    m_weight.reshape({m_weight.shape(2), m_weight.shape(3), m_weight.shape(0), m_weight.shape(1)});

    float *p_m_weight = m_weight.get_raw();
    for (size_t g = 0; g < m_group; g++)
      for (size_t h = 0; h < m_kernel_size; h++)
        for (size_t w = 0; w < m_kernel_size; w++)
        {
          m_weight.at(h, w, g, 0) = weight.at(g, 0, h, w);
        }

    if (m_has_bias)
      m_bias.load(in);
  }

  void groupconv2d_relu_permute_msa::single_conv_group(const tensor_float &x, tensor_float &y, std::size_t i, std::shared_ptr<TensorParams> param) const
  {
    assert(m_group % 8 == 0);
    size_t height = y.shape(1), width = y.shape(2);
    size_t in_height = x.shape(1), in_width = x.shape(2);
    const float *p_x = x.get_raw();
    const float *p_m_weight = m_weight.get_raw();
    const float *p_m_bias = m_bias.get_raw();
    float *p_y = y.get_raw();
    float *in = (float *)p_x;
    float *out = (float *)p_y;
    float *weight = (float *)p_m_weight;
    float *bias = (float *)p_m_bias;
    v4f32 zero = (v4f32)__msa_ldi_w(0);
    v4f32 fmax = zero;

    for (std::size_t h = 0; h < height; h++)
    {
      std::size_t in_h = m_stride * h;
      std::size_t w = 0;
      for (; w < width / 2 * 2; w += 2)
      {
        for (std::size_t g = 0; g < m_group / 8 * 8; g += 8)
        {
          std::size_t in_w = m_stride * w;
          // [w][g]
          v4f32 vsum[2][2];
          vsum[0][0] = (v4f32)__msa_ldi_w(0);
          vsum[0][1] = vsum[0][0];
          vsum[1][0] = vsum[0][0];
          vsum[1][1] = vsum[0][0];

          for (std::size_t kh = 0; kh < m_kernel_size; ++kh)
          {
            float *in_col_p = in + i * in_height * in_width * m_group + ((in_h + kh) * in_width + in_w) * m_group;
            float *w_col_p = weight + kh * m_kernel_size * m_group;
            v4f32 v_in[2];
            v_in[0] = (v4f32)__msa_ld_w(in_col_p + 0 * m_group + g, 0);
            v_in[1] = (v4f32)__msa_ld_w(in_col_p + 0 * m_group + g, 16);

            v4f32 v_in_next[2];
            float *in_row_p = in_col_p + 1 * m_group;
            for (std::size_t kw = 0; kw < m_kernel_size; ++kw)
            {
              float *w_row_p = w_col_p + kw * m_group;
              v4f32 v_wght[2];
              v_in_next[0] = (v4f32)__msa_ld_w(in_row_p + g, 0);
              v_in_next[1] = (v4f32)__msa_ld_w(in_row_p + g, 16);
              v_wght[0] = (v4f32)__msa_ld_w(w_row_p + g, 0);
              v_wght[1] = (v4f32)__msa_ld_w(w_row_p + g, 16);
              vsum[0][0] = __msa_fmadd_w(v_in[0], v_wght[0], vsum[0][0]);
              vsum[0][1] = __msa_fmadd_w(v_in[1], v_wght[1], vsum[0][1]);
              vsum[1][0] = __msa_fmadd_w(v_in_next[0], v_wght[0], vsum[1][0]);
              vsum[1][1] = __msa_fmadd_w(v_in_next[1], v_wght[1], vsum[1][1]);
              in_row_p = in_row_p + m_group;
              v_in[0] = v_in_next[0];
              v_in[1] = v_in_next[1];
            }
          }
          v4f32 vbias[2];
          vbias[0] = (v4f32)__msa_ld_w(bias + g, 0);
          vbias[1] = (v4f32)__msa_ld_w(bias + g, 16);

          vsum[0][0] = vsum[0][0] + vbias[0];
          vsum[0][1] = vsum[0][1] + vbias[1];
          vsum[1][0] = vsum[1][0] + vbias[0];
          vsum[1][1] = vsum[1][1] + vbias[1];

          vsum[0][0] = __msa_fmax_w(vsum[0][0], zero);
          vsum[0][1] = __msa_fmax_w(vsum[0][1], zero);
          vsum[1][0] = __msa_fmax_w(vsum[1][0], zero);
          vsum[1][1] = __msa_fmax_w(vsum[1][1], zero);

          float *out_base = out + (h * width + w) * m_group + g;
          __msa_st_w((v4i32)vsum[0][0], out_base, 0);
          __msa_st_w((v4i32)vsum[0][1], out_base, 16);
          __msa_st_w((v4i32)vsum[1][0], out_base + m_group, 0);
          __msa_st_w((v4i32)vsum[1][1], out_base + m_group, 16);

          v4f32 vmax[4];
          vmax[0] = __msa_fmax_w(vsum[0][0], vsum[0][1]);
          vmax[1] = __msa_fmax_w(vsum[1][0], vsum[1][1]);
          fmax = __msa_fmax_w(fmax, vmax[0]);
          fmax = __msa_fmax_w(fmax, vmax[1]);
        }
      }
      for (; w < width; w++)
      {
        for (std::size_t g = 0; g < m_group / 8 * 8; g += 8)
        {
          std::size_t in_w = m_stride * w;
          v4f32 vsum[2];
          vsum[0] = (v4f32)__msa_ldi_w(0);
          vsum[1] = vsum[0];
          for (std::size_t kh = 0; kh < m_kernel_size; ++kh)
          {
            float *in_col_p = in + i * in_height * in_width * m_group + ((in_h + kh) * in_width + in_w) * m_group + g;
            float *w_col_p = weight + kh * m_kernel_size * m_group + g;
            for (std::size_t kw = 0; kw < m_kernel_size; ++kw)
            {
              float *in_row_p = in_col_p + kw * m_group;
              float *w_row_p = w_col_p + kw * m_group;
              v4f32 v_in[2];
              v_in[0] = (v4f32)__msa_ld_w(in_row_p, 0);
              v_in[1] = (v4f32)__msa_ld_w(in_row_p, 16);

              v4f32 v_wght[2];
              v_wght[0] = (v4f32)__msa_ld_w(w_row_p, 0);
              v_wght[1] = (v4f32)__msa_ld_w(w_row_p, 16);

              vsum[0] = __msa_fmadd_w(v_in[0], v_wght[0], vsum[0]);
              vsum[1] = __msa_fmadd_w(v_in[1], v_wght[1], vsum[1]);
            }
          }
          v4f32 vbias[2];
          vbias[0] = (v4f32)__msa_ld_w(bias + g, 0);
          vbias[1] = (v4f32)__msa_ld_w(bias + g, 16);

          vsum[0] = vsum[0] + vbias[0];
          vsum[1] = vsum[1] + vbias[1];
          vsum[0] = __msa_fmax_w(vsum[0], zero);
          vsum[1] = __msa_fmax_w(vsum[1], zero);

          float *out_base = out + (h * width + w) * m_group + g;
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
} // namespace tnn
#else
#error unsupport MIPS MSA compile for MSA optimized groupconv2d_relu_permute.
#endif