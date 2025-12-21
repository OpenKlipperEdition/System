#ifndef CONV2D_FIXPOINT_PERMUTE_MSA_H
#define CONV2D_FIXPOINT_PERMUTE_MSA_H

#include <cstring>
#include <type_traits>

#include "layer.h"
#include "avx.h"

namespace tnn {
class conv2d_fixpoint_permute_msa: public layer {
 public:
  conv2d_fixpoint_permute_msa(std::size_t in_channels, std::size_t out_channels, std::size_t kernel_size, std::size_t stride = 1, std::size_t padding = 0, bool bias = true)
      : m_in_channels(in_channels), m_out_channels(out_channels), m_kernel_size(kernel_size), m_stride(stride),
        m_padding(padding), m_has_bias(bias), m_weight({out_channels, in_channels, kernel_size, kernel_size}) {
    if (bias)
      m_bias.resize({out_channels});

  }
  tensorX forward(tensorX &&tensor, thread_pool &threads,std::shared_ptr<TensorParams> param = nullptr) const;
  void load(std::istream &in);
 private:

  void single_conv(const tensor_float &x, tensor_float &y) const;
  void single_conv_1_1(const tensor_int8 &x, tensor_float &y,std::shared_ptr<TensorParams> param) const;
  std::size_t m_in_channels, m_out_channels, m_kernel_size, m_stride, m_padding;
  bool m_has_bias;
  tensor_int8 m_weight;
  tensor_float m_bias;
  tensor_float factor_w;

};
}

#endif /* CONV2D_FIXPOINT_PERMUTE_MSA_H */
