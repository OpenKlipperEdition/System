#ifndef CONV2D_NORM_FIXPOINT_PERMUTE_MSA_H
#define CONV2D_NORM_FIXPOINT_PERMUTE_MSA_H

#include <cstring>
#include <type_traits>

#include "layer.h"

namespace tnn
{
  class conv2d_norm_fixpoint_permute_msa : public layer
  {
  public:
    conv2d_norm_fixpoint_permute_msa(std::size_t in_channels, std::size_t out_channels, std::size_t kernel_size, std::size_t stride = 1, std::size_t padding = 0, bool bias = true,
                                     double eps = 1e-05, float momentum = 0.1,
                                     bool affine = true, bool track_running_stats = true)
        : m_in_channels(in_channels), m_out_channels(out_channels), m_kernel_size(kernel_size), m_stride(stride),
          m_padding(padding), m_has_bias(bias), m_weight({out_channels, in_channels, kernel_size, kernel_size}),
          m_features(out_channels),
          m_eps(eps),
          m_momentum(momentum),
          m_affine(affine),
          m_track_running_stats(track_running_stats),
          m_norm_weight({m_features}),
          m_norm_bias({m_features}),
          m_norm_mean({m_features}),
          m_norm_var({m_features})
    {
      if (bias)
        m_bias.resize({out_channels});
      m_alpha.resize({m_features});
      m_beta.resize({m_features});
      m_norm_batch_num.resize({2});
      assert(padding == 0);
    }
    tensorX forward(tensorX &&tensor, thread_pool &threads, std::shared_ptr<TensorParams> param = nullptr) const;
    void load(std::istream &in);

  private:
    void single_conv(const tensor_float &x, tensor_float &y, std::size_t i, std::size_t out, std::shared_ptr<TensorParams> param) const;
    void single_conv_1_1(const tensor_int8 &ix, tensor_float &y, std::size_t i, std::size_t out, std::shared_ptr<TensorParams> param) const;

    std::size_t m_in_channels, m_out_channels, m_kernel_size, m_stride, m_padding;
    bool m_has_bias;
    tensor_int8 m_weight;
    tensor_float m_bias;

    tensor_float factor_w;

    std::size_t m_features;
    tensor_float m_norm_weight, m_norm_bias, m_norm_mean, m_norm_var, m_norm_batch_num;
    tensor_float m_alpha, m_beta;
    double m_eps, m_momentum;
    bool m_affine, m_track_running_stats;
  };
} // namespace tnn

#endif /* CONV2D_NORM_FIXPOINT_PERMUTE_MSA_H */
