#ifndef BIDIRECTIONALLSTM_FIXPOINT_MSA_H
#define BIDIRECTIONALLSTM_FIXPOINT_MSA_H

#include <cmath>
#include <cstring>
#include <type_traits>

#include "layer.h"

namespace tnn {
class BidirectionalLSTM_fixpoint_msa :public layer {
 public:
  struct Parameters{
    tensor_float b_ih;
    tensor_float b_hh;
    tensor_uint16 w_hh;
    tensor_int8 w_ih_i8;
    tensor_float factor_w_ih;

    Parameters(std::size_t input_size,size_t hidden_size){
      w_ih_i8 = tensor_int8({4*hidden_size,input_size});
      w_hh = tensor_uint16({4*hidden_size,hidden_size});

      b_ih = tensor_float({4*hidden_size});
      b_hh = tensor_float({4*hidden_size});
    }
    void load(std::istream &in)
    {
      w_ih_i8.load(in);
      w_hh.load(in);
      b_ih.load(in);
      b_hh.load(in);
    }
  };
  float cvt16fto32f(uint16_t w) const;
  uint16_t cvt32fto16f(float x) const;
  BidirectionalLSTM_fixpoint_msa(std::size_t input_size,std::size_t hidden_size) :szHidden(hidden_size),szInput(input_size)
  {
    params.push_back(std::make_shared<Parameters>(input_size,hidden_size));
    params.push_back(std::make_shared<Parameters>(input_size,hidden_size));
  }
  virtual ~BidirectionalLSTM_fixpoint_msa(){
    params.clear();
  }

  void load(std::istream &in);
  tensorX forward(tensorX &&x, thread_pool &threads,std::shared_ptr<TensorParams> param = nullptr) const ;
 private:
  void single_lstm_fwd(const tensor_int8 &in, tensor_uint16 &out, tensor_float factor_in_8, const std::shared_ptr<Parameters> &p,std::size_t off,std::size_t s, std::size_t e) const;
  void single_lstm_bwd(const tensor_int8 &in, tensor_uint16 &out, tensor_float factor_in_8, const std::shared_ptr<Parameters> &p,std::size_t off,std::size_t s, std::size_t e) const;

  std::vector<std::shared_ptr<Parameters>> params;
  std::size_t szHidden;
  std::size_t szInput;
};
}

#endif /* BIDIRECTIONALLSTM_FIXPOINT_MSA_H */
