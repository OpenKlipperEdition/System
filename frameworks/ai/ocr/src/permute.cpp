#include "layers/layer.h"
#include "layers/permute.h"
#include <assert.h>
namespace tnn {

tensorX permute::forward(tensorX &&tensor, thread_pool &threads,std::shared_ptr<TensorParams> param) const {
    tensor_float x = tensor.getTensor();
    std::vector<std::size_t> shape;
    assert(x.ndim() == 4);
    std::size_t batch = x.shape(0), channels = x.shape(1);
    std::size_t height = x.shape(2), width = x.shape(3);

    std::size_t swap_index[4];
    std::size_t *c = &swap_index[m_shape[1]];
    std::size_t *h = &swap_index[m_shape[2]];
    std::size_t *w = &swap_index[m_shape[3]];

    tensor_float y{batch, x.shape(m_shape[1]), x.shape(m_shape[2]),x.shape(m_shape[3])};

    for (int n = 0; n < batch; n++)
      for (*c = 0; *c < channels; *c = *c + 1)
        for (*h = 0; *h < height; *h = *h + 1)
          for (*w = 0; *w < width; *w = *w + 1) {
            y.at(n, swap_index[1], swap_index[2], swap_index[3])
                = x.at(n, *c, *h, *w);

          }

    return y;
  }
}
