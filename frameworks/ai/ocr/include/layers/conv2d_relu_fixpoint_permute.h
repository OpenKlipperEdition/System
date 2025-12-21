#ifndef CONV2D_RELU_FIXPOINT_PERMUTE_H
#define CONV2D_RELU_FIXPOINT_PERMUTE_H

#include <cstring>
#include <type_traits>
#include <limits>
#include "layer.h"

namespace tnn {
class conv2d_relu_fixpoint_permute: public layer {
 public:

  conv2d_relu_fixpoint_permute(std::size_t in_channels, std::size_t out_channels, std::size_t kernel_size, std::size_t stride = 1, std::size_t padding = 0, bool bias = true)
      : m_in_channels(in_channels), m_out_channels(out_channels), m_kernel_size(kernel_size), m_stride(stride),
        m_padding(padding), m_has_bias(bias), m_weight({out_channels, in_channels, kernel_size, kernel_size}) {
    if (bias)
      m_bias.resize({out_channels});

    assert(bias);
  }
  tensorX forward(tensorX &&tensor, thread_pool &threads,std::shared_ptr<TensorParams> param = nullptr) const {
    tensor_uint8 x;
    tensor.getTensor(x);
    assert(x.ndim() == 4 && x.shape(1) == m_in_channels);
    std::size_t n = x.shape(0), start = 0;
    double step;
    if (m_padding) {
      tensor_uint8 temp{n, x.shape(1), x.shape(2) + 2 * m_padding, x.shape(3) + 2 * m_padding};
      for(std::size_t i = 0;i < n * x.shape(1) * x.shape(2);i++){
        std::size_t c = i / x.shape(2), h = i % x.shape(2);
        memcpy(temp.get_raw(c, h + m_padding, m_padding), x.get_raw(c, h, 0), x.shape(3) * sizeof(uint8_t));
      }
      x = std::move(temp);
    }

    tensor_float y{n, (x.shape(2) - m_kernel_size) / m_stride + 1,
          (x.shape(3) - m_kernel_size) / m_stride + 1, m_out_channels};


    single_conv(x, y,param);
    return y;
  }
  void load(std::istream &in) {
    m_weight.load(in);
    if (m_has_bias)
      m_bias.load(in);
    std::size_t out_channels = m_weight.shape(0);
    std::size_t in_channels = m_weight.shape(1);
    std::size_t kernel_size = m_weight.shape(2);

    for(std::size_t i = 0;i < out_channels;i++){
      float b = 0;
      for(std::size_t j = 0;j < in_channels;j++){
        for(std::size_t n = 0;n < kernel_size;n++){
          for(std::size_t m = 0;m < kernel_size;m++){
            float f = m_weight.at(i,j,n,m);
            b += -f;
            m_weight.at(i,j,n,m) = f * (2.0 / 255.0);
          }
        }
      }
      m_bias.at(i) = m_bias.at(i) + b;
    }
  }
 private:
  void single_conv(const tensor_uint8 &x, tensor_float &y,std::shared_ptr<TensorParams> param) const
  {
    std::size_t height = y.shape(1), width = y.shape(2);
    std::size_t out_channels = y.shape(3);
    std::size_t x_shape2 = x.shape(2), x_shape3 = x.shape(3);
    std::size_t m_weight_shape1 = m_weight.shape(1), m_weight_shape2 = m_weight.shape(2), m_weight_shape3 = m_weight.shape(3);
    tensor_int32 maxInput(m_bias.shape());
    const uint8_t* p_x = x.get_raw();
    const float* p_m_weight = m_weight.get_raw();
    // int max = 0;
    float fmax = 0;
    for(std::size_t i = 0;i < out_channels;i++) {
      for (std::size_t h = 0; h < height; ++h) {
        std::size_t hs = m_stride * h;
        for (std::size_t w = 0; w < width; ++w) {

          float sum = 0;
          float f = 0;
          std::size_t ws = m_stride * w;
          for (std::size_t in = 0; in < m_in_channels; ++in){
            for (std::size_t kh = 0; kh < m_kernel_size; ++kh)
              for (std::size_t kw = 0; kw < m_kernel_size; ++kw){
                //sum += (float)x.at(0, in, hs + kh, ws + kw) * m_weight.at(i, in, kh, kw);
                sum += (float)p_x[(in * x_shape2 + hs + kh) * x_shape3 + ws + kw] * \
                  p_m_weight[((i * m_weight_shape1 + in) * m_weight_shape2 + kh) * m_weight_shape3 + kw];
              }
          }
          float d = m_bias.at(i) + sum;
          if (d < 0)
            d = 0;
          if(d > fmax)
            fmax = d;
          y.at(0, h, w, i) = d;
        }
      }
    }
  }
  std::size_t m_in_channels, m_out_channels, m_kernel_size, m_stride, m_padding;
  bool m_has_bias;

  tensor_float m_weight;
  tensor_float m_bias;
};
}


#endif //CONV2D_RELU_FIXPOINT_PERMUTE_H
