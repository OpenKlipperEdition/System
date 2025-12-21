#ifndef LINEAR_F16_MSA_H
#define LINEAR_F16_MSA_H

#include <cmath>
#include <cstring>
#include <type_traits>

#include "layer.h"

namespace tnn {
class Linear_f16_msa :public layer {
 public:
  float cvt16fto32f(uint16_t w) const;
  uint16_t cvt32fto16f(float x) const;
  Linear_f16_msa(std::size_t hidden_size,std::size_t nOut) :szHidden(hidden_size),szOut(nOut)
  {
    m_weight = tensor_uint16{nOut,2 * hidden_size};
    m_bias = tensor_float{nOut};
  }
  virtual ~Linear_f16_msa(){
  }

  void load(std::istream &in);
  tensorX forward(tensorX &&x, thread_pool &threads,std::shared_ptr<TensorParams> param = nullptr) const ;
 private:
  void single_linear(const tensor_uint16 &in, tensor_float &out,std::size_t s, std::size_t e) const;

  tensor_float m_bias;
  tensor_uint16 m_weight;
  std::size_t szOut;
  std::size_t szHidden;
};
}

#endif /* LINEAR_F16_MSA_H */
