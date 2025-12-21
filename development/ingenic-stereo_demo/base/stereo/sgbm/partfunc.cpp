#undef PRELOAD
#undef GET_MIN
#undef LOAD_Lr
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

#define PRELOAD(base,off) asm volatile ("pref 0,%1(%0)\n" ::"r"(base),"i"(off):"memory")
void part1func(int k,int y,int fullDP,int D,int SH2,int SW2,int width1,size_t costBufSize,CostType* hsumBuf,CostType* C,CostType* hsumAdd)
{
	int x,d;
	int hsumBufNRows = SH2*2 + 2;
	v8i16 v_sw2 = __msa_fill_h(SW2 + 1);
	CostType* pixDiff = hsumBuf + costBufSize*hsumBufNRows;
	for(d = 0;d < D / 8 * 8;d+=8){
		v8i16 v_pixdiff = __msa_ld_h(&pixDiff[d],0);
		v8i16 v_hsumadd = v_pixdiff * v_sw2;
		__msa_st_h(v_hsumadd,&hsumAdd[d],0);
	}

	for(;d < D;d++){
		hsumAdd[d] =  pixDiff[d] *(SW2 + 1);
	}

	for( x = D; x <= SW2*D; x += D )
	{
		for( d = 0; d < D / 8 * 8; d+=8 )
		{
			v8i16 v_pixDiff = __msa_ld_h(&pixDiff[x + d],0);
			v8i16 v_hsumAdd = __msa_ld_h(&hsumAdd[d],0);
			v_hsumAdd += v_pixDiff;
			__msa_st_h(v_hsumAdd,&hsumAdd[d],0);

		}
		for( ; d < D; d++ )
			hsumAdd[d] = (CostType)(hsumAdd[d] + pixDiff[x + d]);
	}
	if( y > 0 )
	{
		CostType* hsumSub = hsumBuf + (std::max(y - SH2 - 1, 0) % hsumBufNRows)*costBufSize;
		CostType* Cprev = !fullDP ? C : C - costBufSize;
		for( x = D; x < width1*D; x += D )
		{
			CostType* pixAdd = pixDiff + std::min(x + SW2*D, (width1-1)*D);
			CostType* pixSub = pixDiff + std::max(x - (SW2+1)*D, 0);
			{
				d = 0;
				CostType* hsumAdd_x = hsumAdd + x;
				CostType* Cprev_x = Cprev + x;
				CostType* C_x = C + x;
				CostType* hsumSub_x = hsumSub + x;
#if 1
				for(; d < D / 8 * 8; d+=8)
				{
					v8i16 v_pixAdd = __msa_ld_h(&pixAdd[d],0);
					//PRELOAD(&pixAdd[d],32);
					v8i16 v_pixSub = __msa_ld_h(&pixSub[d],0);
					//PRELOAD(&pixSub[d],32);
					v8i16 v_hv = v_pixAdd - v_pixSub;
					v8i16 v_hsumAdd0 = __msa_ld_h(&hsumAdd_x[d - D],0);
					v_hv += v_hsumAdd0;

					v8i16 v_Cprev = __msa_ld_h(&Cprev_x[d],0);
					PRELOAD(&Cprev_x[d],32);
					v8i16 v_C = v_Cprev + v_hv;
					v8i16 v_hsumSub = __msa_ld_h(&hsumSub_x[d],0);
					v_C -= v_hsumSub;
					__msa_st_h(v_hv,&hsumAdd_x[d],0);

					PRELOAD(&pixAdd[d],32);
					PRELOAD(&pixSub[d],32);

					__msa_st_h(v_C,&C_x[d],0);
				}
#else
				for(; d < D / 16 * 16; d+=16)
				{
					v8i16 v_pixAdd = __msa_ld_h(&pixAdd[d],0);
					v8i16 v_pixAdd_1 = __msa_ld_h(&pixAdd[d],16);

					v8i16 v_pixSub = __msa_ld_h(&pixSub[d],0);
					v8i16 v_pixSub_1 = __msa_ld_h(&pixSub[d],16);

					v8i16 v_hv = v_pixAdd - v_pixSub;
					v8i16 v_hv_1 = v_pixAdd_1 - v_pixSub_1;

					v8i16 v_hsumAdd0 = __msa_ld_h(&hsumAdd_x[d - D],0);
					v8i16 v_hsumAdd0_1 = __msa_ld_h(&hsumAdd_x[d - D],16);
					v_hv += v_hsumAdd0;
					v_hv_1 += v_hsumAdd0_1;

					v8i16 v_Cprev = __msa_ld_h(&Cprev_x[d],0);
					v8i16 v_Cprev_1 = __msa_ld_h(&Cprev_x[d],16);
					PRELOAD(&Cprev_x[d],64);

					v8i16 v_C = v_Cprev + v_hv;
					v8i16 v_C_1 = v_Cprev_1 + v_hv_1;

					v8i16 v_hsumSub = __msa_ld_h(&hsumSub_x[d],0);
					v8i16 v_hsumSub_1 = __msa_ld_h(&hsumSub_x[d],16);

					v_C -= v_hsumSub;
					v_C_1 -= v_hsumSub_1;

					__msa_st_h(v_hv,&hsumAdd_x[d],0);
					__msa_st_h(v_hv_1,&hsumAdd_x[d],16);

					PRELOAD(&pixAdd[d],64);
					PRELOAD(&pixSub[d],64);

					__msa_st_h(v_C,&C_x[d],0);
					__msa_st_h(v_C_1,&C_x[d],16);
				}

#endif
				for(; d < D; d++ )
				{
					int hv = hsumAdd[x + d] = (CostType)(hsumAdd[x - D + d] + pixAdd[d] - pixSub[d]);
					C[x + d] = (CostType)(Cprev[x + d] + hv - hsumSub[x + d]);
				}
			}

		}
	}else{
		for( x = D; x < width1*D; x += D )
		{
			CostType* pixAdd = pixDiff + std::min(x + SW2*D, (width1-1)*D);
			CostType* pixSub = pixDiff + std::max(x - (SW2+1)*D, 0);
			for( d = 0; d < D / 8 * 8; d+=8 )
			{
				v8i16 v_hsumadd = __msa_ld_h(&hsumAdd[x - D + d],0);
				v8i16 v_pixadd = __msa_ld_h(&pixAdd[d],0);
				PRELOAD(&pixAdd[d],16);
				v_hsumadd += v_pixadd;
				v8i16 v_pixsub = __msa_ld_h(&pixSub[d],0);
				PRELOAD(&pixSub[d],16);
				v_hsumadd -= v_pixsub;
				// PRELOAD(&pixAdd[d],32);
				// PRELOAD(&pixSub[d],32);
				__msa_st_h(v_hsumadd,&hsumAdd[x + d],0);
			}

			for(; d < D; d++ )
				hsumAdd[x + d] = (CostType)(hsumAdd[x - D + d] + pixAdd[d] - pixSub[d]);
		}
	}
	if( y == 0 )
	{
		if(k == 0){
			int scale = SH2 + 1;
			v8i16 v_scale = __msa_fill_h(scale);
			x = 0;
			for( x = 0; x < width1*D / 8 * 8; x+= 8 )
			{
				v8i16 v_C = __msa_ld_h(&C[x],0);
				v8i16 v_hsumadd = __msa_ld_h(&hsumAdd[x],0);
				PRELOAD(&hsumAdd[x],32);
				v4i32 v_mul[2];
				v_mul[0] = __msa_mulsr_w(v_hsumadd,v_scale);
				v_mul[1] = __msa_mulsl_w(v_hsumadd,v_scale);
				v_mul[0] = __msa_accsr_w(v_mul[0],v_C);
				v_mul[1] = __msa_accsl_w(v_mul[1],v_C);
				v_C = __msa_pckev_h(v_mul[1],v_mul[0]);
				__msa_st_h(v_C,&C[x],0);
			}
			for(; x < width1*D; x++ )
				C[x] = (CostType)(C[x] + hsumAdd[x]*scale);
		}else{
			for( x = 0; x < width1*D / 8 * 8; x+= 8 )
			{
				v8i16 v_C = __msa_ld_h(&C[x],0);
				v8i16 v_hsumadd = __msa_ld_h(&hsumAdd[x],0);
				PRELOAD(&hsumAdd[x],32);
				v_C += v_hsumadd;
				__msa_st_h(v_C,&C[x],0);
			}
			for(; x < width1*D; x++ )
				C[x] = (CostType)(C[x] + hsumAdd[x]);
		}
	}
}
typedef short DispType;
void part3func(IMat &disp1,CostType* disp2cost,DispType* disp2ptr,int y,int D,int width1,int INVALID_DISP_SCALED,int P1,int P2,int npasses,CostType** minLr,CostType **Lr,int minD,CostType* S,CostType* C,int uniquenessRatio,int minX1,int DISP_SCALE, long long int &time_loop31)
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

	int x,d;
	int width = disp1.cols;
	int D2 = D+16, NRD2 = NR2*D2;
	const CostType MAX_COST = SHRT_MAX;
	DispType* disp1ptr = disp1.ptr<DispType>(y);

        long long time_temp2 = GetMicrosecondCount();

       // setvalue(disp1ptr, disp2ptr, disp2cost, width, INVALID_DISP_SCALED,MAX_COST );
        asm("ssnop\n\t");
        asm("ssnop\n\t");
#if 0
        for(x = 0; x < width; x++ )
        {
          disp1ptr[x] = disp2ptr[x] = (DispType)INVALID_DISP_SCALED;
          disp2cost[x] = MAX_COST;
        }
#elif 1
        v8i16 v_INVALID_DISP_SCALED = __msa_fill_h(INVALID_DISP_SCALED);
        v8i16 v_MAX_COST = __msa_fill_h(MAX_COST);

        const int unroll = 8;
        for(x = 0; x < width/unroll * unroll; x += unroll)
        {
          __msa_st_h(v_INVALID_DISP_SCALED, &disp1ptr[x], 0);
          // __msa_st_h(v_INVALID_DISP_SCALED, &disp1ptr[x], 16);
          __msa_st_h(v_INVALID_DISP_SCALED, &disp2ptr[x], 0);
          // __msa_st_h(v_INVALID_DISP_SCALED, &disp2ptr[x], 16);
          __msa_st_h(v_MAX_COST, &disp2cost[x], 0);
          // __msa_st_h(v_MAX_COST, &disp2cost[x], 16);
        }
#else
        v8i16 v_INVALID_DISP_SCALED = __msa_fill_h(INVALID_DISP_SCALED);
        v8i16 v_MAX_COST = __msa_fill_h(MAX_COST);
        const int unroll = 8;
        for(x = 0; x < width/unroll * unroll; x += unroll)
        {
          __msa_st_h(v_INVALID_DISP_SCALED, &disp1ptr[x], 0);
          // __msa_st_h(v_INVALID_DISP_SCALED, &disp1ptr[x], 16);
        }
        for(x = 0; x < width/unroll * unroll; x += unroll)
        {
          __msa_st_h(v_INVALID_DISP_SCALED, &disp2ptr[x], 0);
          // __msa_st_h(v_INVALID_DISP_SCALED, &disp2ptr[x], 16);
        }
        for(x = 0; x < width/unroll * unroll; x += unroll)
        {
          __msa_st_h(v_MAX_COST, &disp2cost[x], 0);
          // __msa_st_h(v_MAX_COST, &disp2cost[x], 16);
        }
#endif
        asm("ssnop\n\t");
        asm("ssnop\n\t");
        time_loop31 =  time_loop31 + GetMicrosecondCount() - time_temp2;
	v8i16 __d8 = {0, 1, 2, 3, 4, 5, 6, 7};
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
			if (useSIMD)
			{
			  v8i16 _P1 = __msa_fill_h((short)P1);
			  v8i16 L0, L0_p, L0_n;
			  v8i16 L0_f;
			  
			  PRELOAD(Cp, 0);
			  PRELOAD(Lr_p0 - 1, 0);
			  v8i16 _delta0 = __msa_fill_h((short)delta0);
			  L0_p = __msa_ld_h(Lr_p0 - 1, 0);
			  v8i16 _minL0 = __msa_fill_h((short)minL0);
			  PRELOAD(Sp, 0);
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
			else
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
}

void part2func(int x1, int x2, int dx, int NR2, int D2, int P2, int NRD2, int MAX_COST, int P1, int D,
	CostType **Lr, CostType* C,CostType *S, CostType **minLr)
{
	int x;
	int d;

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

		  if (useSIMD)
		    {
		      v8i16 _P1 = __msa_fill_h((short)P1);
		      PRELOAD(Cp, 0);
		      v8i16 _delta0 = __msa_fill_h((short)delta0);
		      v8i16 _delta1 = __msa_fill_h((short)delta1);
		      v8i16 _delta2 = __msa_fill_h((short)delta2);
		      v8i16 _delta3 = __msa_fill_h((short)delta3);
		      PRELOAD(Lr_p0, -2);		      
		      v8i16 _minL0 = __msa_fill_h((short)MAX_COST);
		      v8i16 _minL1 = __msa_fill_h((short)MAX_COST);		      
		      v8i16 _minL2 = __msa_fill_h((short)MAX_COST);
		      v8i16 _minL3 = __msa_fill_h((short)MAX_COST);

		      v8i16 L0, L1, L2, L3;
		      v8i16 L0_p, L1_p, L2_p, L3_p;
		      v8i16 L0_n, L1_n, L2_n, L3_n;
		      v8i16 L0_f, L1_f, L2_f, L3_f;
		
		      for (d = 0; d < D; d += 8)
			{
			  v8i16 Cpd = __msa_ld_h(const_cast<CostType *>(Cp + d), 0);

			  //
			  PRELOAD(Lr_p1 + d, 46);
			  L0_p = __msa_ld_h(Lr_p0 + d, -2);
			  PRELOAD(Lr_p2 + d, 46);			  
			  L1_p = __msa_ld_h(Lr_p1 + d, -2);
			  PRELOAD(Lr_p3 + d, 46);
			  L2_p = __msa_ld_h(Lr_p2 + d, -2);
			  L3_p = __msa_ld_h(Lr_p3 + d, -2);
			  PRELOAD(Lr_p0 + d, 46);

			  L0   = __msa_ld_h(Lr_p0 + d, 0);
			  L1   = __msa_ld_h(Lr_p1 + d, 0);
			  L2   = __msa_ld_h(Lr_p2 + d, 0);
			  L3   = __msa_ld_h(Lr_p3 + d, 0);

			  L0_n = __msa_ld_h(Lr_p0 + d, +2);
			  L1_n = __msa_ld_h(Lr_p1 + d, +2);
			  L2_n = __msa_ld_h(Lr_p2 + d, +2);
			  L3_n = __msa_ld_h(Lr_p3 + d, +2);

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

			  PRELOAD(Sp + d, 32);

			  _minL0 = __msa_min_s_h(_minL0, L0);
			  _minL1 = __msa_min_s_h(_minL1, L1);
			  _minL2 = __msa_min_s_h(_minL2, L2);
			  _minL3 = __msa_min_s_h(_minL3, L3);

			  v8i16 Sval = __msa_ld_h(Sp + d, 0);

			  PRELOAD(Cp + d, 32);

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
}
