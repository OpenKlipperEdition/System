#ifndef MAXPOOL2D_PERMUTE_H
#define MAXPOOL2D_PERMUTE_H

#include <limits>
#include "layer.h"
#include <math.h>
#include<stdio.h>
namespace tnn {
class maxpool2d_permute: public layer {
 public:
 maxpool2d_permute(std::size_t kernel_size, std::size_t stride = 0, std::size_t padding = 0, std::size_t dilation = 1) {

   if (stride == 0)
     stride = kernel_size;

   m_kernel_size = {kernel_size, kernel_size};
   m_stride = {stride, stride};
   m_padding = {padding, padding};
   m_dilation = {dilation, dilation};

   if ((m_padding[0] && m_kernel_size[0]/m_padding[0] < 2)
       ||(m_padding[1] && m_kernel_size[1]/m_padding[1] < 2)) {
     printf("RuntimeError: pad should be smaller than half of kernel size, but got padW = %ld, padH = %ld, kW = %ld, kH = %ld\n",
            m_padding[0], m_padding[1], m_kernel_size[0], m_kernel_size[1]);
     abort();
   }
 }

 maxpool2d_permute(std::initializer_list<std::size_t> kernel_size,
           std::initializer_list<std::size_t> stride,
           std::initializer_list<std::size_t> padding
           ,std::initializer_list<std::size_t> dilation = {1, 1}/* , bool ceil_mode = false */)
 : m_kernel_size(kernel_size), m_stride(stride), m_padding(padding), m_dilation(dilation) {

   if (m_stride[0] == 0)
     m_stride[0] = m_kernel_size[0];
   if (m_stride[1] == 0)
     m_stride[1] = m_kernel_size[1];

   if ((m_padding[0] && m_kernel_size[0]/m_padding[0] < 2)
       ||(m_padding[1] && m_kernel_size[1]/m_padding[1] < 2)) {
     printf("RuntimeError: pad should be smaller than half of kernel size, but got padW = %ld, padH = %ld, kW = %ld, kH = %ld\n",
            m_padding[0], m_padding[1], m_kernel_size[0], m_kernel_size[1]);
     abort();
   }
 }
 tensorX forward(tensorX &&tensor, thread_pool &threads,std::shared_ptr<TensorParams> param = nullptr) const;

 private:
 void single_maxpool2d(const tensor_float &x, tensor_float &y, std::size_t i, std::size_t channels) const;

 std::vector<std::size_t> m_kernel_size, m_stride, m_padding,  m_dilation;
};

}

#endif


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
