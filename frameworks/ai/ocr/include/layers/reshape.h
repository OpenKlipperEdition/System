#ifndef RESHAPE_H
#define RESHAPE_H

#include "layer.h"

namespace tnn {
class reshape: public layer {
 public:
  reshape(std::initializer_list<std::size_t> shape): m_shape(shape), m_size(1) {
    for (std::size_t i = 0; i < m_shape.size(); ++i)
      m_size *= m_shape[i];
  }
  tensorX forward(tensorX &&tensor, thread_pool &threads,std::shared_ptr<TensorParams> param = nullptr) const {
    tensor_float x = tensor.getTensor();
    /* std::vector<std::size_t> shape; */
    /* shape.reserve(m_shape.size() + 1); */

    /* shape.emplace_back(x.size() / m_size); */
    /* shape.insert(shape.end(), m_shape.begin(), m_shape.end()); */
    /* x.reshape(x.shape(0), shape.begin(), shape.end()); */
    x.reshape({x.shape(0), x.shape(1), x.shape(3)});
    return x;
  }

 private:
  std::vector<std::size_t> m_shape;
  std::size_t m_size;
};
}

#endif
