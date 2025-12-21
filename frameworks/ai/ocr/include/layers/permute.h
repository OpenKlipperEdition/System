#ifndef PERMUTE_H
#define PERMUTE_H

#include "layer.h"
#include <assert.h>
namespace tnn {
class permute: public layer {
 public:
  permute(std::initializer_list<std::size_t> shape): m_shape(shape), m_size(1) {

    /* for (std::size_t i = 0; i < m_shape.size(); ++i) */
    /*       m_size *= m_shape[i]; */
  }
  tensorX forward(tensorX &&tensor, thread_pool &threads,std::shared_ptr<TensorParams> param = nullptr) const;
 private:
  std::vector<std::size_t> m_shape;
  std::size_t m_size;
};
}

#endif
