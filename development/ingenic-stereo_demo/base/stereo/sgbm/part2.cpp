#define PRELOAD(base,off) asm volatile ("pref 0,%1(%0)\n" ::"r"(base),"i"(off):"memory")
#define PRESTORE(base,off) asm volatile ("pref 30,%1(%0)\n" ::"r"(base),"i"(off):"memory")

//#include "part2-retry.cpp"


#define GET_MIN(_lp, _lc, _ln, _P, _delta, _cost)	\
  do{							\
    _lp = __msa_min_s_h(_lp, _ln);			\
    _lc = __msa_min_s_h(_lc, _lp + _P);			\
    _lc = __msa_min_s_h(_lc, _delta);			\
    _lc = (_lc - _delta) + _cost;			\
  }while(0)

#define LOAD_Lr(_addr, _lf, _lp, _lc, _ln, _d)	\
  do{						\
    _lf = __msa_ld_h(_addr + _d - 1 + 8, 0);	\
    _lc   = __msa_vsldi_b(_lp, _lf, 2);		\
    _ln = __msa_vsldi_b(_lp, _lf, 4);		\
  }while(0)

static inline void
part_getmin(v8i16 L_p, v8i16 &L_f, v8i16 &L, v8i16 &L_1, v8i16 _delta,
	    v8i16 Cpd, v8i16 Cpd1, v8i16 _P1, CostType *Lr_p0, int d)
{
  v8i16 L_n, L_n1, L_p1;
  L_p1 = __msa_ld_h(Lr_p0 + d - 1, 16);
  L_f  = __msa_ld_h(Lr_p0 + d - 1, 32);
  L    = __msa_vsldi_b(L_p,  L_p1, 2);
  L_n  = __msa_vsldi_b(L_p,  L_p1, 4);
  L_1  = __msa_vsldi_b(L_p1, L_f, 2);
  L_n1 = __msa_vsldi_b(L_p1, L_f, 4);
  PRELOAD(Lr_p0 + d - 1 + 16, 32);
  GET_MIN(L_p, L, L_n, _P1, _delta, Cpd);
  GET_MIN(L_p1, L_1, L_n1, _P1, _delta, Cpd1);
}

void compute_msa(IMat &img1, IMat &img2, IMat &disp1, SgbmDisparityParam *params)
{
  static const uchar LSBTab[] =
    {
      0, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
      5, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
      6, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
      5, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
      7, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
      5, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
      6, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
      5, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0
    };
  static const v8u16 v_LSB = {0x1, 0x2, 0x4, 0x8, 0x10, 0x20, 0x40, 0x80};
  useSIMD = 1;
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

  long long time_temp = 0, time_temp2 =0, time_loop1 = 0, time_loop0 = 0, time_loop2 = 0, time_loop3 = 0, time_loop31 = 0, time_loop4 = 0;

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

#include "part1.cpp"
	  
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
	  // if (0)
	  //   {
	  //     part2( x1, x2,  dx,  NR2,  D2,  P2,  NRD2, MAX_COST, P1, D, Lr, C, S, minLr);
	  //   }
	  // else 
            if (0)
	    {
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
#if 0
#include "tmp.cpp"
#else

		  if (useSIMD)
		    {
		      v8i16 _P1 = __msa_fill_h((short)P1);
		      PRELOAD(Lr_p0 - 1, 0);
		      v8i16 _delta0 = __msa_fill_h((short)delta0);
		      v8i16 _delta1 = __msa_fill_h((short)delta1);
		      v8i16 _delta2 = __msa_fill_h((short)delta2);
		      v8i16 _delta3 = __msa_fill_h((short)delta3);
		      v8i16 _minL0 = __msa_fill_h((short)MAX_COST);
		      v8i16 _minL1 = __msa_fill_h((short)MAX_COST);		      
		      v8i16 _minL2 = __msa_fill_h((short)MAX_COST);
		      v8i16 _minL3 = __msa_fill_h((short)MAX_COST);

		      v8i16 L0, L1, L2, L3;
		      v8i16 L0_p, L1_p, L2_p, L3_p;
		      v8i16 L0_n, L1_n, L2_n, L3_n;
		      v8i16 L0_f, L1_f, L2_f, L3_f;
		      // PRELOAD(Lr_p1 -1 , 0);
		      L0_p = __msa_ld_h(Lr_p0 - 1, 0);
		      L1_p = __msa_ld_h(Lr_p1 - 1, 0);
		      L2_p = __msa_ld_h(Lr_p2 - 1, 0);
		      L3_p = __msa_ld_h(Lr_p3 - 1, 0);

		
		      for (d = 0; d < D; d += 8)
			{
			  v8i16 Cpd = __msa_ld_h(const_cast<CostType *>(Cp + d), 0);
			  

			  //
			  PRELOAD(Lr_p1 + d - 1 + 8, 32);
			  L0_p = __msa_ld_h(Lr_p0 + d - 1, 0);
			  PRELOAD(Lr_p2 + d - 1 + 8, 32);			  
			  L1_p = __msa_ld_h(Lr_p1 + d - 1, 0);
			  PRELOAD(Lr_p3 + d - 1 + 8, 32);
			  L2_p = __msa_ld_h(Lr_p2 + d - 1, 0);
			  L3_p = __msa_ld_h(Lr_p3 + d - 1, 0);
			  PRELOAD(Lr_p0 + d - 1 + 8, 32);

			  L0   = __msa_ld_h(Lr_p0 + d, 0);
			  L1   = __msa_ld_h(Lr_p1 + d, 0);
			  L2   = __msa_ld_h(Lr_p2 + d, 0);
			  L3   = __msa_ld_h(Lr_p3 + d, 0);

			  L0_n = __msa_ld_h(Lr_p0 + d + 1, 0);
			  L1_n = __msa_ld_h(Lr_p1 + d + 1, 0);
			  L2_n = __msa_ld_h(Lr_p2 + d + 1, 0);
			  L3_n = __msa_ld_h(Lr_p3 + d + 1, 0);

			  // LOAD_Lr(Lr_p0, L0_f, L0_p, L0, L0_n, d);
			  // PRELOAD(Lr_p0 + d - 1 + 8, 32);
			  // LOAD_Lr(Lr_p1, L1_f, L1_p, L1, L1_n, d);
			  // PRELOAD(Lr_p1 + d - 1 + 8, 32);
			  // LOAD_Lr(Lr_p2, L2_f, L2_p, L2, L2_n, d);
			  // PRELOAD(Lr_p2 + d - 1 + 8, 32);
			  // LOAD_Lr(Lr_p3, L3_f, L3_p, L3, L3_n, d);
			  // PRELOAD(Lr_p3 + d - 1 + 8, 32);

			  GET_MIN(L0_p, L0, L0_n, _P1, _delta0, Cpd);
			  GET_MIN(L1_p, L1, L1_n, _P1, _delta1, Cpd);
			  GET_MIN(L2_p, L2, L2_n, _P1, _delta2, Cpd);
			  GET_MIN(L3_p, L3, L3_n, _P1, _delta3, Cpd);

			  __msa_st_h(L0, Lr_p + d, 0) ;
			  __msa_st_h(L1, Lr_p + d + D2, 0);
			  __msa_st_h(L2, Lr_p + d + D2*2, 0);
			  __msa_st_h(L3, Lr_p + d + D2*3, 0);
			  PRELOAD(Sp + d, 16);

			  _minL0 = __msa_min_s_h(_minL0, L0);
			  _minL1 = __msa_min_s_h(_minL1, L1);
			  _minL2 = __msa_min_s_h(_minL2, L2);
			  _minL3 = __msa_min_s_h(_minL3, L3);

			  v8i16 Sval = __msa_ld_h(Sp + d, 0);
			  PRELOAD(Cp + d, 16);

			  L0 = __msa_adds_s_h(L0, L1);
			  L2 = __msa_adds_s_h(L2, L3);
			  Sval = __msa_adds_s_h(Sval, L0);
			  Sval = __msa_adds_s_h(Sval, L2);

			  // L0 = L0 + L1;
			  // L2 = L2 + L3;
			  // Sval = Sval + L0;
			  // Sval = Sval + L2;
			  __msa_st_h ( Sval, Sp + d, 0);
			}
		      v8i16 t01l, t01h, t23l, t23h;
		      t01l = __msa_pckev_h(_minL1, _minL0);
		      t01h = __msa_pckod_h(_minL1, _minL0);
		      t23l = __msa_pckev_h(_minL3, _minL2);
		      t23h = __msa_pckod_h(_minL3, _minL2);
		      v8i16 t01 = __msa_min_s_h (t01l, t01h);
		      v8i16 t23 = __msa_min_s_h (t23l, t23h);
		      v8i16 t0123l = __msa_pckev_h(t23, t01);
		      v8i16 t0123h = __msa_pckod_h(t23, t01);
		      v8i16 t0 = __msa_min_s_h(t0123l, t0123h);
		      v8i16 _min = __msa_shf_h(t0, 0B10110001);
		      _min = __msa_min_s_h(_min, t0);
		      _min = __msa_pckev_h(_min, _min);
		      __msa_stext_d(_min, 0, &minLr[0][xm], 0);
		    }
#endif
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
	    }// part 2
	  else
	    {
	      part2func( x1, x2,  dx,  NR2,  D2,  P2,  NRD2, MAX_COST, P1, D, Lr, C, S, minLr);
	    }
	  time_loop2 = time_loop2 + GetMicrosecondCount() - time_temp;

	  time_temp = GetMicrosecondCount();
	  if( pass == npasses )
	    {
#if 0	      
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
		      int pp1 = 0, pp2 = 1, pp3 =0;
		      // if (pp1)
		      // 	{
		      // 	  register v8i16 _P1 = __msa_fill_h((short)P1);
		      // 	  register v8i16 _delta0 = __msa_fill_h((short)delta0);
		      // 	  register v8i16 _minL0 = __msa_fill_h((short)minL0);
		      // 	  register v8i16 _minS = __msa_fill_h(MAX_COST), _bestDisp = __msa_ldi_h(-1);
		      // 	  register v8i16 _d8 = {0, 1, 2, 3, 4, 5, 6, 7}, _8 = __msa_ldi_h(8);
		      // 	  register v8i16 L0, L0_p, L0_n;

		      // 	  L0_p = __msa_ld_h(Lr_p0 - 1, 0);

		      // 	  register v8i16 L0_f;
		      // 	  for (d = 0; d < D; d += 8)
		      // 	    {
		      // 	     register v8i16 Cpd = __msa_ld_h(const_cast<CostType *>(Cp) + d, 0);


		      // 	      L0_p = __msa_ld_h(Lr_p0 + d - 1, 0);
		      // 	      L0 = __msa_ld_h(Lr_p0 + d, 0);
		      // 	      L0_n = __msa_ld_h(Lr_p0 + d + 1, 0);

		      // 	      L0_p = __msa_min_s_h(L0_p, L0_n);
		      // 	      L0 = __msa_min_s_h(L0, L0_p + _P1);
		      // 	      L0 = __msa_min_s_h(L0, _delta0);
		      // 	      L0 = L0 - _delta0 + Cpd;

		      // 	      PRELOAD(Sp + d, 16);
		      // 	      __msa_st_h(L0, Lr_p + d,  0);
		      // 	      _minL0 = __msa_min_s_h(_minL0, L0);

		      // 	      // Sval
		      // 	      L0 = L0 + __msa_ld_h(Sp + d, 0);
		      // 	      PRELOAD(Cp + d , 16);
		      // 	      __msa_st_h(L0, Sp + d, 0);

		      // 	      PRELOAD(Lr_p0 + d , 16);

			      
		      // 	      v8i16 mask = _minS > L0;
		      // 	      _minS = __msa_min_s_h(_minS, L0);
		      // 	      _bestDisp = _bestDisp ^ ((_bestDisp ^ _d8) & mask);
		      // 	      _d8 += _8;
		      // 	    }

		      // 	  short bestDispBuf[8];
		      // 	  __msa_st_h (_bestDisp, bestDispBuf, 0);
		      // 	  v8i16 t1, t2;
		      // 	  t1 = __msa_pckev_h(_minS, _minL0);
		      // 	  t2 = __msa_pckod_h(_minS, _minL0);
		      // 	  t1 = __msa_min_s_h(t1, t2);
		      // 	  t2 = __msa_shf_h(t1, 0B10110001);
		      // 	  t1 = __msa_min_s_h(t1, t2);
		      // 	  t2 = __msa_shf_h(t1, 0B01001110);
		      // 	  t1 = __msa_min_s_h(t1, t2);
		      // 	  minLr[0][xm] = t1[0];
		      // 	  minS = t1[4];
		      // 	  v8i16 ss = __msa_shf_w(t1, 0B10101010);
		      // 	  v8i16 minMask = __msa_ceq_h(ss, _minS);
		      // 	  v8u16 minBit = minMask & v_LSB;

		      // 	  v2i64 sum = (v2i64)__msa_hadd_u_w(minBit, minBit);
		      // 	  sum = __msa_hadd_u_d (sum, sum);
		      // 	  int idx = sum[0] + sum[1];
		      // 	  bestDisp = bestDispBuf[LSBTab[idx]];
		      // 	}
		      // else if (pp2)
			{
			  v8i16 _P1 = __msa_fill_h((short)P1);
			  v8i16 L0, L0_p, L0_n;
			  v8i16 L0_f;
			  
			  PRELOAD(Cp, 0);
			  // PRELOAD(Lr_p0 - 1, 0);
			  v8i16 _delta0 = __msa_fill_h((short)delta0);
			  L0_p = __msa_ld_h(Lr_p0 - 1, 0);
			  v8i16 _minL0 = __msa_fill_h((short)minL0);
			  // PRELOAD(Sp, 0);			  
			  v8i16 _minS = __msa_fill_h(MAX_COST), _bestDisp = __msa_ldi_h(-1);
			  v8i16 _d8 = {0, 1, 2, 3, 4, 5, 6, 7}, _8 = __msa_ldi_h(8);
			  for (d = 0; d < D; d += 16)
			    {
			      v8i16 Cpd = __msa_ld_h(const_cast<CostType *>(Cp) + d, 0);
			      v8i16 Cpd1 = __msa_ld_h(const_cast<CostType *>(Cp) + d, 16);
			      v8i16 L0_n, L0_n1, L0_p1;
			      v8i16 L0_1;

			      L0_p = __msa_ld_h(Lr_p0 + d, -2);
			      L0_p1= __msa_ld_h(Lr_p0 + d, 14);
			      
			      L0   = __msa_ld_h(Lr_p0 + d, 0);
			      L0_n = __msa_ld_h(Lr_p0 + d, +2);

			      L0_1 = __msa_ld_h(Lr_p0 + d, 16);
			      L0_n1= __msa_ld_h(Lr_p0 + d, 18);

			      GET_MIN(L0_p, L0, L0_n, _P1, _delta0, Cpd);
			      GET_MIN(L0_p1, L0_1, L0_n1, _P1, _delta0, Cpd1);

			      PRELOAD(Sp + d, 48);

			      // Sval
			      v8i16 Sval = __msa_ld_h(Sp + d, 0);
			      v8i16 Sval_1 = __msa_ld_h(Sp + d, 16);
			      __msa_st_h(L0,  Lr_p + d, 0) ;
			      __msa_st_h(L0_1,Lr_p + d, 16) ;

			      PRELOAD(Lr_p0 + d - 1, 48);

			      _minL0 = __msa_min_s_h(_minL0, L0);
			      _minL0 = __msa_min_s_h(_minL0, L0_1);
			      L0 = __msa_adds_s_h(L0, Sval);
			      L0_1 = __msa_adds_s_h(L0_1, Sval_1);
			      __msa_st_h(L0, Sp + d, 0);
			      __msa_st_h(L0_1, Sp + d, 16);
			      PRELOAD(Cp + d, 64);

			      v8i16 mask = _minS > L0;
			      _minS = __msa_min_s_h(_minS, L0);
			      _bestDisp = _bestDisp ^ ((_bestDisp ^ _d8) & mask);
			      _d8 += _8;
			      mask = _minS > L0_1;
			      _minS = __msa_min_s_h(_minS, L0_1);
			      _bestDisp = _bestDisp ^ ((_bestDisp ^ _d8) & mask);
			      _d8 += _8;
			    }

			  short bestDispBuf[8];
			  __msa_st_h (_bestDisp, bestDispBuf, 0);

			  v8i16 t1, t2;
			  t1 = __msa_pckev_h(_minS, _minL0);
			  t2 = __msa_pckod_h(_minS, _minL0);
			  t1 = __msa_min_s_h(t1, t2);
			  t2 = __msa_shf_h(t1, 0B10110001);
			  t1 = __msa_min_s_h(t1, t2);
			  t2 = __msa_shf_h(t1, 0B01001110);
			  t1 = __msa_min_s_h(t1, t2);
			  minLr[0][xm] = t1[0];
			  minS = t1[4];
			  v8i16 ss = __msa_shf_w(t1, 0B10101010);
			  v8i16 minMask = __msa_ceq_h(ss, _minS);
			  v8u16 minBit = minMask & v_LSB;

			  v2i64 sum = (v2i64)__msa_hadd_u_w(minBit, minBit);
			  sum = __msa_hadd_u_d (sum, sum);
			  int idx = sum[0] + sum[1];
			  bestDisp = bestDispBuf[LSBTab[idx]];
			}
		      // else if (pp3)
		      // 	{
		      // 	  v8i16 _P1 = __msa_fill_h((short)P1);
		      // 	  v8i16 _delta0 = __msa_fill_h((short)delta0);
		      // 	  v8i16 _minL0 = __msa_fill_h((short)minL0);
		      // 	  v8i16 _minS = __msa_fill_h(MAX_COST), _bestDisp = __msa_ldi_h(-1);
		      // 	  v8i16 _d8 = {0, 1, 2, 3, 4, 5, 6, 7}, _8 = __msa_ldi_h(8);
		      // 	  for (d = 0; d < D; d += 8)
		      // 	    {
		      // 	      v8i16 Cpd = __msa_ld_h(const_cast<CostType *>(Cp) + d, 0);
		      // 	      v8i16 L0, L0_p, L0_n;
		      // 	      L0_p = __msa_ld_h(Lr_p0 + d - 1, 0);
		      // 	      L0 = __msa_ld_h(Lr_p0 + d, 0);
		      // 	      L0_n = __msa_ld_h(Lr_p0 + d + 1, 0);

		      // 	      L0_p = __msa_min_s_h(L0_p, L0_n);
		      // 	      L0 = __msa_min_s_h(L0, L0_p + _P1);
		      // 	      L0 = __msa_min_s_h(L0, _delta0);
		      // 	      L0 = L0 - _delta0 + Cpd;

		      // 	      __msa_st_h(L0, Lr_p + d,  0);
		      // 	      _minL0 = __msa_min_s_h(_minL0, L0);

		      // 	      // Sval
		      // 	      v8i16 Sval = __msa_ld_h(Sp + d, 0);
		      // 	      L0 = L0 + Sval;
		      // 	      __msa_st_h(L0, Sp + d, 0);

		      // 	      v8i16 mask = _minS > L0;
		      // 	      _minS = __msa_min_s_h(_minS, L0);
		      // 	      _bestDisp = _bestDisp ^ ((_bestDisp ^ _d8) & mask);
		      // 	      _d8 += _8;
		      // 	    }
		      // 	  short bestDispBuf[8];
		      // 	  __msa_st_h (_bestDisp, bestDispBuf, 0);
		      // 	  v8i16 t1, t2;
		      // 	  t1 = __msa_pckev_h(_minS, _minL0);
		      // 	  t2 = __msa_pckod_h(_minS, _minL0);
		      // 	  t1 = __msa_min_s_h(t1, t2);
		      // 	  t2 = __msa_shf_h(t1, 0B10110001);
		      // 	  t1 = __msa_min_s_h(t1, t2);
		      // 	  t2 = __msa_shf_h(t1, 0B01001110);
		      // 	  t1 = __msa_min_s_h(t1, t2);
		      // 	  minLr[0][xm] = t1[0];
		      // 	  minS = t1[4];
		      // 	  v8i16 ss = __msa_shf_w(t1, 0B10101010);
		      // 	  v8i16 minMask = __msa_ceq_h(ss, _minS);
		      // 	  v8u16 minBit = minMask & v_LSB;

		      // 	  v2i64 sum = (v2i64)__msa_hadd_u_w(minBit, minBit);
		      // 	  sum = __msa_hadd_u_d (sum, sum);
		      // 	  int idx = sum[0] + sum[1];
		      // 	  bestDisp = bestDispBuf[LSBTab[idx]];			  
		      // 	}
		      // else
		      // 	{
		      // 	  for( d = 0; d < D; d++ )
		      // 	    {
		      // 	      int L0 = Cp[d] + std::min((int)Lr_p0[d], std::min(Lr_p0[d-1] + P1, std::min(Lr_p0[d+1] + P1, delta0))) - delta0;

		      // 	      Lr_p[d] = (CostType)L0;
		      // 	      minL0 = std::min(minL0, L0);

		      // 	      int Sval = Sp[d] = saturate_cast<CostType>(Sp[d] + L0);
		      // 	      if( Sval < minS )
		      // 		{
		      // 		  minS = Sval;
		      // 		  bestDisp = d;
		      // 		}
		      // 	    }
		      // 	  minLr[0][xm] = (CostType)minL0;
		      // 	}
		    }
		  else
		    {
		      if (useSIMD)
			{
			  v8i16 _minS = __msa_fill_h(MAX_COST), _bestDisp = __msa_ldi_h(-1);
			  v8i16 _d8 = {0, 1, 2, 3, 4, 5, 6, 7}, _8 = __msa_ldi_h(8);
			  for (d = 0; d < D; d += 8)
			    {
			      // Sval
			      v8i16 L0 = __msa_ld_h(Sp + d, 0);
			      v8i16 mask = _minS > L0;
			      _minS = __msa_min_s_h(_minS, L0);
			      _bestDisp = _bestDisp ^ ((_bestDisp ^ _d8) & mask);
			      _d8 += _8;
			    }
			  v8i16 t1, t2;
			  t1 = __msa_shf_h(_minS, 0B10110001);
			  t1 = __msa_min_s_h(t1, _minS);
			  t2 = __msa_shf_w(t1, 0B10110001);
			  t1 = __msa_min_s_h(t1, t2);
			  t2 = __msa_shf_w(t1, 0B01001110);
			  t1 =__msa_min_s_h(t1, t2);
			  minS = t1[0];

			  v8i16 ss = t1;
			  v8i16 v_mask = __msa_ceq_h(ss, _minS);
			  v8i16 _shrt_max = __msa_fill_h(SHRT_MAX);
			  _bestDisp = __msa_bmnz_v(_bestDisp, _shrt_max, v_mask);

			  t1 = __msa_shf_h(_bestDisp, 0B10110001);
			  t1 = __msa_min_s_h(t1, _bestDisp);
			  t2 = __msa_shf_w(t1, 0B10110001);
			  t1 = __msa_min_s_h(t1, t2);
			  t2 = __msa_shf_w(t1, 0B01001110);
			  t1 =__msa_min_s_h(t1, t2);
			  bestDisp = t1[0];
			}
		      else
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
#else
	      part3func(disp1,disp2cost,disp2ptr,y,D,width1,INVALID_DISP_SCALED,P1,P2,npasses,minLr,Lr,minD,S,C,uniquenessRatio,minX1,DISP_SCALE, time_loop31);
#endif	      

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

      printf("time_loop0 %lld time_loop1 %lld loop2 %lld loop3 %lld loop31 %lld loop4 %lld \n", time_loop0, time_loop1, time_loop2, time_loop3, time_loop31, time_loop4);
    }
  buffer.release();
}
