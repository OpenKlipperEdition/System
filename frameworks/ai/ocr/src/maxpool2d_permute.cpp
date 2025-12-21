#include <limits>
#include <math.h>
#include<stdio.h>
#include "layers/layer.h"
#include "layers/maxpool2d_permute.h"

namespace tnn{
tensorX maxpool2d_permute::forward(tensorX &&tensor, thread_pool &threads,std::shared_ptr<TensorParams> param) const {
                tensor_float x = tensor.getTensor();
   assert(x.ndim() == 4);
   std::size_t n = x.shape(0), channels = x.shape(3), start = 0;
   assert(n == 1);

   std::size_t outputH = (x.shape(1) + 2*m_padding[0] - m_dilation[0]*(m_kernel_size[0]-1) - 1) / m_stride[0] + 1;
   std::size_t outputW = (x.shape(2) + 2*m_padding[1] - m_dilation[1]*(m_kernel_size[1]-1) - 1) / m_stride[1] + 1;
   assert(outputH > 0 && outputW > 0);

   tensor_float y{n, outputH , outputW, channels};

   single_maxpool2d(x, y, 0, channels);

   return y;
 }

void maxpool2d_permute::single_maxpool2d(const tensor_float &x, tensor_float &y, std::size_t i, std::size_t channels) const {
   std::size_t height = y.shape(1), width = y.shape(2);
   std::size_t dH = m_stride[0];
   std::size_t dW = m_stride[1];
   std::size_t dilationH = m_dilation[0];
   std::size_t dilationW = m_dilation[1];
   std::size_t in_height = x.shape(1), in_width = x.shape(2);
   const float* p_x = x.get_raw();
   for (std::size_t h = 0; h < height; ++h) {
     for (std::size_t w = 0; w < width; ++w) {
       for (std::size_t c = 0; c < channels; ++c){
        int32_t hstart = h * dH - m_padding[0];
        int32_t wstart = w * dW - m_padding[1];
        int32_t hend = std::min(hstart + (m_kernel_size[0] - 1)* dilationH + 1, in_height);
        int32_t wend = std::min(wstart + (m_kernel_size[1] - 1)* dilationW + 1, in_width);

        while (hstart < 0)
          hstart += dilationH;
        while (wstart < 0)
          wstart += dilationW;

        float max = -std::numeric_limits<float>::max(), value;

        for (std::size_t kh = 0; kh < hend - hstart ; kh += dilationH)
          for (std::size_t kw = 0; kw < wend - wstart; kw += dilationW) {
            //value = x.at(i, c, hstart + kh, wstart + kw);
            value = p_x[((i * in_height + hstart + kh) * in_width + wstart + kw) * channels + c];

            if (value > max || isnan(value))
              max = value;
          }
        y.at(i, h, w, c) = max;
       }
     }
   }
 }
}


    /* T outputSize = div_rtn<T>( */
    /*     inputSize + pad_l + pad_r - dilation * (kernelSize - 1) - 1 + */
    /*     (ceil_mode ? stride - 1 : 0), stride) + 1; */
    /* if (pad_l) { */
    /*     // ensure that the last pooling starts inside the image */
    /*     // needed to avoid problems in ceil mode */
    /*     if ((outputSize - 1) * stride >= inputSize + pad_l) */
    /*       --outputSize; */
    /* } */
    /* return outputSize; */
