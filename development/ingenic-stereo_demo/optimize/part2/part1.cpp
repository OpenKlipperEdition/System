#if !PART1_OP
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

#elif 0
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
	    v8i16 v_sw2 = __msa_fill_h(SW2 + 1);
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
		      for(; d < D / 8 * 8; d+=8)
			{
			  v8i16 v_pixAdd = __msa_ld_h(&pixAdd[d],0);
			  v8i16 v_pixSub = __msa_ld_h(&pixSub[d],0);
			  v8i16 v_hv = v_pixAdd - v_pixSub;
			  v8i16 v_hsumAdd0 = __msa_ld_h(&hsumAdd[x - D + d],0);
			  v_hv += v_hsumAdd0;

			  v8i16 v_Cprev = __msa_ld_h(&Cprev[x + d],0);
			  v8i16 v_C = v_Cprev + v_hv;
			  v8i16 v_hsumSub = __msa_ld_h(&hsumSub[x + d],0);
			  v_C -= v_hsumSub;

			  __msa_st_h(v_hv,&hsumAdd[x + d],0);

			  // PRELOAD(&pixAdd[d],16);
			  // PRELOAD(&pixSub[d],16);

			  __msa_st_h(v_C,&C[x + d],0);
			}

		      for(; d < D; d++ )
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
		    CostType* pixAdd = pixDiff + std::min(x + SW2*D, (width1-1)*D);
		    CostType* pixSub = pixDiff + std::max(x - (SW2+1)*D, 0);
		    for( d = 0; d < D / 8 * 8; d+=8 )
		      {
			v8i16 v_hsumadd = __msa_ld_h(&hsumAdd[x - D + d],0);
			v8i16 v_pixadd = __msa_ld_h(&pixAdd[d],0);
			v_hsumadd += v_pixadd;
			v8i16 v_pixsub = __msa_ld_h(&pixSub[d],0);
			//PRELOAD(&pixAdd[d],16);
			v_hsumadd -= v_pixsub;
			// PRELOAD(&pixAdd[d],32);
			// PRELOAD(&pixSub[d],32);
			__msa_st_h(v_hsumadd,&hsumAdd[x + d],0);
		      }

		    for(; d < D; d++ )
		      hsumAdd[x + d] = (CostType)(hsumAdd[x - D + d] + pixAdd[d] - pixSub[d]);
		  }
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
		  v4i32 v_mul[2];
		  v_mul[0] = __msa_mulsr_w(v_hsumadd,v_scale);
		  v_mul[1] = __msa_mulsl_w(v_hsumadd,v_scale);
		  PRELOAD(&hsumAdd[x],32);
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
		  v_C += v_hsumadd;
		  PRELOAD(&hsumAdd[x],32);
		  __msa_st_h(v_C,&C[x],0);
		}
	      for(; x < width1*D; x++ )
		C[x] = (CostType)(C[x] + hsumAdd[x]);
	    }
	  }
      }
    // also, clear the S buffer
    {
      v8i16 v_zero = __msa_ldi_b(0);
      for( k = 0; k < width1*D / 32 * 32; k+=32 ){
	__msa_st_h(v_zero,&S[k],0);
	__msa_st_h(v_zero,&S[k],16);
	__msa_st_h(v_zero,&S[k],32);
	__msa_st_h(v_zero,&S[k],48);
      }
      for(; k < width1*D; k++ )
	S[k] = 0;
    }
  } // end pass == 1

#elif 1

#define PRELOAD(base,off) asm volatile ("pref 0,%1(%0)\n" ::"r"(base),"i"(off):"memory")

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

	    part1func(k,y,fullDP,D,SH2,SW2,width1,costBufSize,hsumBuf,C,hsumAdd);
	  }

      }
    // also, clear the S buffer
    {
      v8i16 v_zero = __msa_ldi_b(0);
      for( k = 0; k < width1*D / 32 * 32; k+=32 ){
	__msa_st_h(v_zero,&S[k],0);
	__msa_st_h(v_zero,&S[k],16);
	__msa_st_h(v_zero,&S[k],32);
	__msa_st_h(v_zero,&S[k],48);
      }
      for(; k < width1*D; k++ )
	S[k] = 0;
    }
  } // end pass == 1

#else
#define PRELOAD(base,off) asm volatile ("pref 0,%1(%0)\n" ::"r"(base),"i"(off):"memory")

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

      }
    // also, clear the S buffer
    {
      v8i16 v_zero = __msa_ldi_b(0);
      for( k = 0; k < width1*D / 32 * 32; k+=32 ){
	__msa_st_h(v_zero,&S[k],0);
	__msa_st_h(v_zero,&S[k],16);
	__msa_st_h(v_zero,&S[k],32);
	__msa_st_h(v_zero,&S[k],48);
      }
      for(; k < width1*D; k++ )
	S[k] = 0;
    }
  } // end pass == 1
#endif
