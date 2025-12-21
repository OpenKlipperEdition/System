#ifndef LAYER_H
#define LAYER_H

#include <cstdint>
#include "threadpool.h"
#include "tensor/tensor.h"
#include "sys/time.h"
#if defined(__mips)
#include <msa.h>
#endif
static long long getSystemTime()
{
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return tv.tv_sec * 1000000LL + tv.tv_usec;
}

namespace tnn {

struct TensorParams
{
  float factor_w;
  float input_max;
  tensor_float pre_input;
  tensor_float pre_lstm_out;
  tensor_uint16 pre_input_f16;

  bool rollback_recog;
  int feature_off;
};

class layer{
 public:
  virtual tensorX forward(tensorX &&tensor, thread_pool &threads,std::shared_ptr<TensorParams> param = nullptr) const = 0;

  virtual void load(std::istream &in) {}
  virtual ~layer() {};

  void float_to_uint8(const tensor_float &x, tensor_int8 &ix, float fw) const
  {
    const float *p_x = x.get_raw();
    int8_t *p_ix = ix.get_raw();
    std::size_t i = 0;
    #if defined(__mips_msa)
    #define FLOAT_UINT_1GROUP(_a, _b, _c, ia0, _result)                             \
    do                                                                              \
    {                                                                               \
        v4f32 v_ix_f[4];                                                            \
        v_ix_f[0] = (v4f32)__msa_fmul_w(_a[ia0], _b);                               \
        v_ix_f[1] = (v4f32)__msa_fmul_w(_a[ia0+1], _b);                             \
        v_ix_f[2] = (v4f32)__msa_fmul_w(_a[ia0+2], _b);                             \
        v_ix_f[3] = (v4f32)__msa_fmul_w(_a[ia0+3], _b);                             \
        v4u32 v_ix_u[4];                                                            \
        v_ix_u[0] = (v4u32)__msa_ftrunc_u_w(v_ix_f[0]);                             \
        v_ix_u[1] = (v4u32)__msa_ftrunc_u_w(v_ix_f[1]);                             \
        v_ix_u[2] = (v4u32)__msa_ftrunc_u_w(v_ix_f[2]);                             \
        v_ix_u[3] = (v4u32)__msa_ftrunc_u_w(v_ix_f[3]);                             \
        v16i8 v_temp[2];                                                            \
        v_temp[0] = (v16i8)__msa_vshf_b(_c, (v16i8)v_ix_u[1], (v16i8)v_ix_u[0]);    \
        v_temp[1] = (v16i8)__msa_vshf_b(_c, (v16i8)v_ix_u[3], (v16i8)v_ix_u[2]);    \
        _result = (v16i8)__msa_pckev_d((v2i64)v_temp[1], (v2i64)v_temp[0]);         \
    } while (0);
    float *p_x_offset = (float *)p_x;
    v4f32 v_fw = (v4f32)__msa_fill_w(*((int *)&fw));
    unsigned char vshf_wd[16] = {0, 4, 8, 12, 16, 20, 24, 28, 0, 0, 0, 0, 0, 0, 0, 0};
    v16i8 v_vshf_wd = (v16i8)__msa_ld_b(vshf_wd, 0);
    for (; i < x.size() / 32 * 32; i += 32)
    {
        v4f32 v_x[8];
        v_x[0] = (v4f32)__msa_ld_w(p_x_offset, 0);
        v_x[1] = (v4f32)__msa_ld_w(p_x_offset, 16);
        v_x[2] = (v4f32)__msa_ld_w(p_x_offset, 32);
        v_x[3] = (v4f32)__msa_ld_w(p_x_offset, 48);
        v_x[4] = (v4f32)__msa_ld_w(p_x_offset, 64);
        v_x[5] = (v4f32)__msa_ld_w(p_x_offset, 80);
        v_x[6] = (v4f32)__msa_ld_w(p_x_offset, 96);
        v_x[7] = (v4f32)__msa_ld_w(p_x_offset, 112);

        v16i8 v_end[2];
        FLOAT_UINT_1GROUP(v_x, v_fw, v_vshf_wd, 0, v_end[0]);
        FLOAT_UINT_1GROUP(v_x, v_fw, v_vshf_wd, 4, v_end[1]);

        __msa_st_b(v_end[0], p_ix + i, 0);
        __msa_st_b(v_end[1], p_ix + i, 16);
        p_x_offset += 32;
    }
    #undef FLOAT_UINT_1GROUP
    #endif
    for ( ; i < x.size(); i++)
    {
      p_ix[i] = p_x[i] * fw;
    }
  }
};



template <typename U = float, typename Allocator = std::allocator<U> >
class layers: public layer{
 public:
  layers(std::initializer_list<std::shared_ptr<layer> > layers)
      : m_layers(layers) {}

  tensorX forward(tensorX &&x,thread_pool &threads,std::shared_ptr<TensorParams> param = nullptr) const {

    if(param == nullptr) {
      param = std::make_shared<TensorParams>();
      param->rollback_recog = false;
    }
    long long start = getSystemTime();

    for (std::size_t i = 0; i < m_layers.size(); ++i) {
      /* long long start = getSystemTime(); */
      x = m_layers[i]->forward(std::move(x), threads, param);
      /* long long finish = getSystemTime() - start; */
      /* printf("layer %2ld, time is %5lld ms\n", i, finish/1000); */
    }
    long long finish = getSystemTime() - start;
    printf("forward time %5lld ms\n", finish/1000);
    return x;
  }

  void load(std::istream &in) {
    for (std::size_t i = 0; i < m_layers.size(); ++i)
      m_layers[i]->load(in);
  }
 private:
  std::vector<std::shared_ptr<layer> > m_layers;
};

}

#endif
