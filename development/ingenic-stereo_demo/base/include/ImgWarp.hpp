#ifndef __IMGWARP_H__
#define __IMGWARP_H__
#include "IMat.hpp"
#include "cvdefs.hpp"

namespace JzStereo_internal {
#define WEBP_INLINE inline
enum InterpolationMasks {
       INTER_BITS      = 5,
       INTER_BITS2     = INTER_BITS * 2,
       INTER_TAB_SIZE  = 1 << INTER_BITS,
       INTER_TAB_SIZE2 = INTER_TAB_SIZE * INTER_TAB_SIZE
     };


static WEBP_INLINE int clip(int v, int min_v, int max_v) {
  return (v < min_v) ? min_v : (v > max_v) ? max_v : v;
}


void remap( JzStereo::IMat &_src, JzStereo::IMat &_dst,
	     JzStereo::IMat &_map1, JzStereo::IMat &_map2,
	     int interpolation = JzStereo_internal::INTER_LINEAR,
	     int borderType = JzStereo_internal::BORDER_CONSTANT /*, const Scalar& borderValue */);

} // JzStereo_internal
#endif // __IMGWARP_H__
