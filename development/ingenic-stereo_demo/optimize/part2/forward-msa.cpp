
#include <stdio.h>
#include <stdint.h>
#include "forward.h"
#include <msa.h>

#include <StereoDisparity.hpp>
#include <stdint.h>
#include <limits.h>
#include <string.h>
#include <saturate_cast.hpp>
#include <test.hpp>
using namespace internal;
using namespace JzStereo;

typedef uint8_t PixType;
typedef short CostType;
typedef short DispType;
static int useSIMD = 1;
enum { NR = 16, NR2 = NR/2 };

static void *alignPtr(void* ptr, uintptr_t align)
{
  uintptr_t mask = align - 1;
  return (void *)(((uintptr_t)ptr + mask) & (~mask));
}
static const int DEFAULT_RIGHT_BORDER = -1;

#define PART0_OP 1
#define PART1_OP 1
#define PART2_OP 1

#include "partfunc.cpp"

#include "part0.cpp"

void compute(IMat &img1, IMat &img2, IMat &disp1, SgbmDisparityParam *params);

#if PART2_OP
#include "part2.cpp"
#else
void compute_msa(IMat &img1, IMat &img2, IMat &disp1, SgbmDisparityParam *params)
{
  const int ALIGN = 16;
  const int DISP_SHIFT = StereoSgbm::DISP_SHIFT;
  const int DISP_SCALE = (1 << DISP_SHIFT);
  const CostType MAX_COST = SHRT_MAX;
  IMat buffer;
  int minD = params->minDisparity, maxD = minD + params->numDisparities;
  Size SADWindowSize;
  SADWindowSize.width = SADWindowSize.height = params->SADWindowSize > 0 ? params->SADWindowSize : 5;

  int ftzero = std::max(params->preFilterCap, 15) | 1;
  int uniquenessRatio = params->uniquenessRatio >= 0 ? params->uniquenessRatio : 10;
  int disp12MaxDiff = params->disp12MaxDiff > 0 ? params->disp12MaxDiff : 1;
  int P1 = params->P1 > 0 ? params->P1 : 2, P2 = std::max(params->P2 > 0 ? params->P2 : 5, P1+1);
  int k, width = disp1.cols, height = disp1.rows;
  int minX1 = std::max(maxD, 0), maxX1 = width + std::min(minD, 0);
  int D = maxD - minD, width1 = maxX1 - minX1;
  int INVALID_DISP = minD - 1, INVALID_DISP_SCALED = INVALID_DISP*DISP_SCALE;
  int SW2 = SADWindowSize.width/2, SH2 = SADWindowSize.height/2;
  bool fullDP = params->mode == StereoSgbm::MODE_HH;


  int npasses = fullDP ? 2 : 1;

  const int TAB_OFS = 256*4, TAB_SIZE = 256 + TAB_OFS*2;
  PixType clipTab[TAB_SIZE];
  for( k = 0; k < TAB_SIZE; k++ )
    clipTab[k] = (PixType)(std::min(std::max(k - TAB_OFS, -ftzero), ftzero) + ftzero);
  
  if( minX1 >= maxX1 )
    {
      // set all disp1 to invalide
      return;
    }

  int D2 = D+16, NRD2 = NR2*D2;
  const int NLR = 2;
  const int LrBorder = NLR - 1;
  size_t costBufSize = width1*D;
  size_t CSBufSize = costBufSize*(fullDP ? height : 1);
  size_t minLrSize = (width1 + LrBorder*2)*NR2, LrSize = minLrSize*D2;
  int hsumBufNRows = SH2*2 + 2;
  size_t totalBufSize = (LrSize + minLrSize)*NLR*sizeof(CostType) + // minLr[] and Lr[]
    costBufSize*(hsumBufNRows + 1)*sizeof(CostType) + // hsumBuf, pixdiff
    CSBufSize*2*sizeof(CostType) + // C, S
    width * 16 * img1.channels() * sizeof(PixType) + // temp buffer for computing per-pixel cost
    width*(sizeof(CostType) + sizeof(DispType)) + 1024; // disp2cost + disp2
  
  if( buffer.empty() || buffer.width * buffer.height * buffer.channels() < totalBufSize )
  {
    int ret = buffer.reserveBuffer(totalBufSize);
    if (ret == false)
    {
      printf("malloc memory %d byte fail\n", totalBufSize);
      return;
    }
  }

  CostType* Cbuf = (CostType*)alignPtr(buffer.ptr<CostType>(), ALIGN);
  CostType* Sbuf = Cbuf + CSBufSize;
  CostType* hsumBuf = Sbuf + CSBufSize;
  CostType* pixDiff = hsumBuf + costBufSize*hsumBufNRows;

  CostType* disp2cost = pixDiff + costBufSize + (LrSize + minLrSize)*NLR;
  DispType* disp2ptr = (DispType*)(disp2cost + width);
  PixType* tempBuf = (PixType*)(disp2ptr + width);

  long long time_temp = 0, time_temp2 =0, time_loop1 = 0, time_loop0 = 0, time_loop2 = 0, time_loop3 = 0, time_loop4 = 0;
  
  // add P2 to every C(x,y). it saves a few operations in the inner loops
  for(k = 0; k < (int)CSBufSize; k++ )
    Cbuf[k] = (CostType)P2;
  
  for( int pass = 1; pass <= npasses; pass++ )
    {
      int x1, y1, x2, y2, dx, dy;
      if( pass == 1 )
        {
	  y1 = 0; y2 = height; dy = 1;
	  x1 = 0; x2 = width1; dx = 1;
        }
      else
        {
	  y1 = height-1; y2 = -1; dy = -1;
	  x1 = width1-1; x2 = -1; dx = -1;
        }

      CostType *Lr[NLR]={0}, *minLr[NLR]={0};

      for( k = 0; k < NLR; k++ )
        {
	  // shift Lr[k] and minLr[k] pointers, because we allocated them with the borders,
	  // and will occasionally use negative indices with the arrays
	  // we need to shift Lr[k] pointers by 1, to give the space for d=-1.
	  // however, then the alignment will be imperfect, i.e. bad for SSE,
	  // thus we shift the pointers by 8 (8*sizeof(short) == 16 - ideal alignment)
	  Lr[k] = pixDiff + costBufSize + LrSize*k + NRD2*LrBorder + 8;
	  memset( Lr[k] - LrBorder*NRD2 - 8, 0, LrSize*sizeof(CostType) );
	  minLr[k] = pixDiff + costBufSize + LrSize*NLR + minLrSize*k + NR2*LrBorder;
	  memset( minLr[k] - LrBorder*NR2, 0, minLrSize*sizeof(CostType) );
        }
      for( int y = y1; y != y2; y += dy )
	{
	  int x, d;
	  DispType* disp1ptr = disp1.ptr<DispType>(y);
	  CostType* C = Cbuf + (!fullDP ? 0 : y*costBufSize);
	  CostType* S = Sbuf + (!fullDP ? 0 : y*costBufSize);
	  time_temp = GetMicrosecondCount();
	  if( pass == 1 ) // compute C on the first pass, and reuse it on the second pass, if any.
            {
	      int dy1 = y == 0 ? 0 : y + SH2, dy2 = y == 0 ? SH2 : dy1;

	      for( k = dy1; k <= dy2; k++ )
                {
		  CostType* hsumAdd = hsumBuf + (std::min(k, height-1) % hsumBufNRows)*costBufSize;
		  if( k < height )
                    {
		      time_temp2 = GetMicrosecondCount();
		      calcPixelCostBT( img1, img2, k, minD, maxD, pixDiff, tempBuf, clipTab, TAB_OFS, ftzero );
		      time_loop0 = time_loop0 + GetMicrosecondCount() - time_temp2; 
		      memset(hsumAdd, 0, D*sizeof(CostType));
		      for( x = 0; x <= SW2*D; x += D )
                        {
			  int scale = x == 0 ? SW2 + 1 : 1;
			  for( d = 0; d < D; d++ )
			    hsumAdd[d] = (CostType)(hsumAdd[d] + pixDiff[x + d]*scale);
                        }

		      if( y > 0 )
                        {
			  const CostType* hsumSub = hsumBuf + (std::max(y - SH2 - 1, 0) % hsumBufNRows)*costBufSize;
			  const CostType* Cprev = !fullDP || y == 0 ? C : C - costBufSize;

			  for( x = D; x < width1*D; x += D )
                            {
			      const CostType* pixAdd = pixDiff + std::min(x + SW2*D, (width1-1)*D);
			      const CostType* pixSub = pixDiff + std::max(x - (SW2+1)*D, 0);
			      {
				for( d = 0; d < D; d++ )
				  {
				    int hv = hsumAdd[x + d] = (CostType)(hsumAdd[x - D + d] + pixAdd[d] - pixSub[d]);
				    C[x + d] = (CostType)(Cprev[x + d] + hv - hsumSub[x + d]);
				  }
			      }
                            }
			}
		      else
			{
			  for( x = D; x < width1*D; x += D )
			    {
			      const CostType* pixAdd = pixDiff + std::min(x + SW2*D, (width1-1)*D);
			      const CostType* pixSub = pixDiff + std::max(x - (SW2+1)*D, 0);

			      for( d = 0; d < D; d++ )
				hsumAdd[x + d] = (CostType)(hsumAdd[x - D + d] + pixAdd[d] - pixSub[d]);
			    }
			}
		    }

		  if( y == 0 )
                    {
		      int scale = k == 0 ? SH2 + 1 : 1;
		      for( x = 0; x < width1*D; x++ )
			C[x] = (CostType)(C[x] + hsumAdd[x]*scale);
                    }
                }
	      // also, clear the S buffer
	      for( k = 0; k < width1*D; k++ )
		S[k] = 0;	      
	    } // end pass == 1
	  time_loop1 =  time_loop1 + GetMicrosecondCount() - time_temp;
      
	  /*
	    [formula 13 in the paper]
	    compute L_r(p, d) = C(p, d) +
	    min(L_r(p-r, d),
	    L_r(p-r, d-1) + P1,
	    L_r(p-r, d+1) + P1,
	    min_k L_r(p-r, k) + P2) - min_k L_r(p-r, k)
	    where p = (x,y), r is one of the directions.
	    we process all the directions at once:
	    0: r=(-dx, 0)
	    1: r=(-1, -dy)
	    2: r=(0, -dy)
	    3: r=(1, -dy)
	    4: r=(-2, -dy)
	    5: r=(-1, -dy*2)
	    6: r=(1, -dy*2)
	    7: r=(2, -dy)
	  */
	  time_temp = GetMicrosecondCount();
	  for( x = x1; x != x2; x += dx )
            {
	      int xm = x*NR2, xd = xm*D2;

	      int delta0 = minLr[0][xm - dx*NR2] + P2, delta1 = minLr[1][xm - NR2 + 1] + P2;
	      int delta2 = minLr[1][xm + 2] + P2, delta3 = minLr[1][xm + NR2 + 3] + P2;

	      CostType* Lr_p0 = Lr[0] + xd - dx*NRD2;
	      CostType* Lr_p1 = Lr[1] + xd - NRD2 + D2;
	      CostType* Lr_p2 = Lr[1] + xd + D2*2;
	      CostType* Lr_p3 = Lr[1] + xd + NRD2 + D2*3;

	      Lr_p0[-1] = Lr_p0[D] = Lr_p1[-1] = Lr_p1[D] =
                Lr_p2[-1] = Lr_p2[D] = Lr_p3[-1] = Lr_p3[D] = MAX_COST;

	      CostType* Lr_p = Lr[0] + xd;
	      const CostType* Cp = C + x*D;
	      CostType* Sp = S + x*D;
	      if (0)
	      {
		v8i16 _P1 = __msa_fill_h((short)P1);
		v8i16 _delta0 = __msa_fill_h((short)delta0);
		v8i16 _delta1 = __msa_fill_h((short)delta1);
		v8i16 _delta2 = __msa_fill_h((short)delta2);
		v8i16 _delta3 = __msa_fill_h((short)delta3);
		v8i16 _minL0 = __msa_fill_h((short)MAX_COST);
		for (d = 0; d < D; d += 8)
		  {
		    v8i16 Cpd = __msa_ld_h(const_cast<CostType *>(Cp + d), 0);
		    v8i16 L0, L1, L2, L3;
		    v8i16 L0_p, L1_p, L2_p, L3_p;
		    v8i16 L0_n, L1_n, L2_n, L3_n;
		    L0_p = __msa_ld_h(Lr_p0 + d - 1, 0);
		    L0   = __msa_ld_h(Lr_p0 + d + 0, 0);
		    L0_n = __msa_ld_h(Lr_p0 + d + 1, 0);

		    L1_p = __msa_ld_h(Lr_p1 + d - 1, 0);
		    L1   = __msa_ld_h(Lr_p1 + d + 0, 0);
		    L1_n = __msa_ld_h(Lr_p1 + d + 1, 0);

		    L2_p = __msa_ld_h(Lr_p2 + d - 1, 0);
		    L2   = __msa_ld_h(Lr_p2 + d + 0, 0);
		    L2_n = __msa_ld_h(Lr_p2 + d + 1, 0);

		    L3_p = __msa_ld_h(Lr_p3 + d - 1, 0);
		    L3   = __msa_ld_h(Lr_p3 + d + 0, 0);
		    L3_n = __msa_ld_h(Lr_p3 + d + 1, 0);

		    L0_p = __msa_min_s_h(L0_p, L0_n);
		    L0   = __msa_min_s_h(L0, L0_p + _P1);
		    L0   = __msa_min_s_h(L0, _delta0);
		    L0   = (L0 - _delta0) + Cpd;

		    L1_p = __msa_min_s_h(L1_p, L1_n);
		    L1	 = __msa_min_s_h(L1, L1_p + _P1);
		    L1	 = __msa_min_s_h(L1, _delta1);
		    L1 	 = (L1 - _delta1) + Cpd;

		    L2_p = __msa_min_s_h(L2_p, L2_n);
		    L2	 = __msa_min_s_h(L2, L2_p + _P1);
		    L2	 = __msa_min_s_h(L2, _delta2);
		    L2	 = (L2 - _delta2) + Cpd;

		    L3_p = __msa_min_s_h(L3_p, L3_n);
		    L3	 = __msa_min_s_h(L3, L3_p + _P1);
		    L3	 = __msa_min_s_h(L3, _delta3);
		    L3	 = (L3 - _delta3) + Cpd;

		    __msa_st_h(L0, Lr_p + d, 0) ;
		    __msa_st_h(L1, Lr_p + d + D2, 0);
		    __msa_st_h(L2, Lr_p + d + D2*2, 0);
		    __msa_st_h(L3, Lr_p + d + D2*3, 0);

		    v8i16 t02l, t02h, t13l, t13h;
		    t02l = __msa_pckev_d(L2, L0);
		    t02h = __msa_pckod_d(L2, L0);
		    t13l = __msa_pckev_d(L3, L1);
		    t13h = __msa_pckod_d(L3, L1);

		    v8i16 t02 = __msa_min_s_h (t02l, t02h);
		    v8i16 t13 = __msa_min_s_h (t13l, t13h);
		    v8i16 t0123l = __msa_ilvev_w(t13, t02);
		    v8i16 t0123h = __msa_ilvod_w(t13, t02);
		    v8i16 t0 = __msa_min_s_h(t0123l, t0123h);
		    _minL0 = __msa_min_s_h(_minL0, t0);

		    v8i16 Sval = __msa_ld_h(Sp + d, 0);
		    // L0 = __msa_adds_s_h(L0, L1);
		    // L2 = __msa_adds_s_h(L2, L3);
		    // Sval = __msa_adds_s_h(Sval, L0);
		    // Sval = __msa_adds_s_h(Sval, L2);
		    L0 = L0 + L1;
		    L2 = L2 + L3;
		    Sval = Sval + L0;
		    Sval = Sval + L2;
		    __msa_st_h ( Sval, Sp + d, 0);
		  }
		v8i16 _min = __msa_shf_h(_minL0, 0B10110001);
		_minL0 = __msa_min_s_h(_minL0, _min);
		_minL0 = __msa_pckev_h(_minL0, _minL0);
		__msa_stext_d(_minL0, 0, &minLr[0][xm], 0);
	      }
	      else{
		int minL0 = MAX_COST, minL1 = MAX_COST, minL2 = MAX_COST, minL3 = MAX_COST;

		for( d = 0; d < D; d++ )
		  {
		    int Cpd = Cp[d], L0, L1, L2, L3;

		    L0 = Cpd + std::min((int)Lr_p0[d], std::min(Lr_p0[d-1] + P1, std::min(Lr_p0[d+1] + P1, delta0))) - delta0;
		    L1 = Cpd + std::min((int)Lr_p1[d], std::min(Lr_p1[d-1] + P1, std::min(Lr_p1[d+1] + P1, delta1))) - delta1;
		    L2 = Cpd + std::min((int)Lr_p2[d], std::min(Lr_p2[d-1] + P1, std::min(Lr_p2[d+1] + P1, delta2))) - delta2;
		    L3 = Cpd + std::min((int)Lr_p3[d], std::min(Lr_p3[d-1] + P1, std::min(Lr_p3[d+1] + P1, delta3))) - delta3;

		    Lr_p[d] = (CostType)L0;
		    minL0 = std::min(minL0, L0);

		    Lr_p[d + D2] = (CostType)L1;
		    minL1 = std::min(minL1, L1);

		    Lr_p[d + D2*2] = (CostType)L2;
		    minL2 = std::min(minL2, L2);

		    Lr_p[d + D2*3] = (CostType)L3;
		    minL3 = std::min(minL3, L3);

		    Sp[d] = saturate_cast<CostType>(Sp[d] + L0 + L1 + L2 + L3);
		  }
		minLr[0][xm] = (CostType)minL0;
		minLr[0][xm+1] = (CostType)minL1;
		minLr[0][xm+2] = (CostType)minL2;
		minLr[0][xm+3] = (CostType)minL3;
	      }
            }
	  time_loop2 = time_loop2 + GetMicrosecondCount() - time_temp;

	  time_temp = GetMicrosecondCount();
	  if( pass == npasses )
            {
	      for( x = 0; x < width; x++ )
                {
		  disp1ptr[x] = disp2ptr[x] = (DispType)INVALID_DISP_SCALED;
		  disp2cost[x] = MAX_COST;
                }

	      for( x = width1 - 1; x >= 0; x-- )
                {
		  CostType* Sp = S + x*D;
		  int minS = MAX_COST, bestDisp = -1;

		  if( npasses == 1 )
                    {
		      int xm = x*NR2, xd = xm*D2;

		      int minL0 = MAX_COST;
		      int delta0 = minLr[0][xm + NR2] + P2;
		      CostType* Lr_p0 = Lr[0] + xd + NRD2;
		      Lr_p0[-1] = Lr_p0[D] = MAX_COST;
		      CostType* Lr_p = Lr[0] + xd;

		      const CostType* Cp = C + x*D;
		      {
			for( d = 0; d < D; d++ )
			  {
			    int L0 = Cp[d] + std::min((int)Lr_p0[d], std::min(Lr_p0[d-1] + P1, std::min(Lr_p0[d+1] + P1, delta0))) - delta0;

			    Lr_p[d] = (CostType)L0;
			    minL0 = std::min(minL0, L0);

			    int Sval = Sp[d] = saturate_cast<CostType>(Sp[d] + L0);
			    if( Sval < minS )
			      {
				minS = Sval;
				bestDisp = d;
			      }
			  }
			minLr[0][xm] = (CostType)minL0;
		      }
                    }
		  else
                    {
		      {
			for( d = 0; d < D; d++ )
			  {
			    int Sval = Sp[d];
			    if( Sval < minS )
			      {
				minS = Sval;
				bestDisp = d;
			      }
			  }
		      }
                    }
		  for( d = 0; d < D; d++ )
                    {
		      if( Sp[d]*(100 - uniquenessRatio) < minS*100 && std::abs(bestDisp - d) > 1 )
			break;
                    }
		  if( d < D )
		    continue;
		  d = bestDisp;
		  int _x2 = x + minX1 - d - minD;
		  if( disp2cost[_x2] > minS )
                    {
		      disp2cost[_x2] = (CostType)minS;
		      disp2ptr[_x2] = (DispType)(d + minD);
                    }

		  if( 0 < d && d < D-1 )
                    {
		      // do subpixel quadratic interpolation:
		      //   fit parabola into (x1=d-1, y1=Sp[d-1]), (x2=d, y2=Sp[d]), (x3=d+1, y3=Sp[d+1])
		      //   then find minimum of the parabola.
		      int denom2 = std::max(Sp[d-1] + Sp[d+1] - 2*Sp[d], 1);
		      d = d*DISP_SCALE + ((Sp[d-1] - Sp[d+1])*DISP_SCALE + denom2)/(denom2*2);
                    }
		  else
		    d *= DISP_SCALE;
		  disp1ptr[x + minX1] = (DispType)(d + minD*DISP_SCALE);
		}
	      for( x = minX1; x < maxX1; x++ )
                {
		  // we round the computed disparity both towards -inf and +inf and check
		  // if either of the corresponding disparities in disp2 is consistent.
		  // This is to give the computed disparity a chance to look valid if it is.
		  int d1 = disp1ptr[x];
		  if( d1 == INVALID_DISP_SCALED )
		    continue;
		  int _d = d1 >> DISP_SHIFT;
		  int d_ = (d1 + DISP_SCALE-1) >> DISP_SHIFT;
		  int _x = x - _d, x_ = x - d_;
		  if( 0 <= _x && _x < width && disp2ptr[_x] >= minD && std::abs(disp2ptr[_x] - _d) > disp12MaxDiff &&
		      0 <= x_ && x_ < width && disp2ptr[x_] >= minD && std::abs(disp2ptr[x_] - d_) > disp12MaxDiff )
		    disp1ptr[x] = (DispType)INVALID_DISP_SCALED;
                }

	    }
	  time_loop3 = time_loop3 + GetMicrosecondCount() - time_temp;
	  // now shift the cyclic buffers
	  time_temp = GetMicrosecondCount();

	  std::swap( Lr[0], Lr[1] );
	  std::swap( minLr[0], minLr[1] );
	  time_loop4 = time_loop4 + GetMicrosecondCount() - time_temp;
	}

      printf("time_loop0 %lld time_loop1 %lld loop2 %lld loop3 %lld loop4 %lld \n", time_loop0, time_loop1, time_loop2, time_loop3, time_loop4);
    }
  buffer.release();
}
#endif
Register B(1,compute_msa);
