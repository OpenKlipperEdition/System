#ifndef GROUPCONV2D_RELU_PERMUTE_MSA_H
#define GROUPCONV2D_RELU_PERMUTE_MSA_H

#include <cstring>
#include <type_traits>

#include "layer.h"

namespace tnn
{
  class groupconv2d_relu_permute_msa : public layer
  {
  public:
    groupconv2d_relu_permute_msa(std::size_t in_channels, std::size_t out_channels, std::size_t kernel_size, std::size_t group, std::size_t stride = 1, std::size_t padding = 0, bool bias = true)
        : m_in_channels(in_channels), m_out_channels(out_channels), m_kernel_size(kernel_size), m_stride(stride),
          m_padding(padding), m_group(group), m_has_bias(bias)
    {
      if (bias)
        m_bias.resize({out_channels});

      if (m_group == 0)
        abort();
      if (m_out_channels % m_group)
      {
        printf("filters must divided by num_group with no remainders!");
        abort();
      }
      if (m_in_channels % m_group)
      {
        printf("input channel must divided by num_group with no remainders!");
        abort();
      }
      assert(m_group == in_channels && m_group == out_channels);
      m_weight.resize({out_channels, m_in_channels / m_group, kernel_size, kernel_size});
    }

    tensorX forward(tensorX &&tensor, thread_pool &threads, std::shared_ptr<TensorParams> param = nullptr) const;
    void load(std::istream &in);

  private:
    void single_conv_group(const tensor_float &x, tensor_float &y, std::size_t i, std::shared_ptr<TensorParams> param) const;

    std::size_t m_in_channels, m_out_channels, m_kernel_size, m_stride, m_padding, m_group;
    bool m_has_bias;
    tensor_float m_weight, m_bias;
  };
} // namespace tnn

#endif /* GROUPCONV2D_RELU_PERMUTE_MSA_H */
