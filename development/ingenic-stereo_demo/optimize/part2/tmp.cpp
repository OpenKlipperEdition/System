
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

    v8i16 L0_p, L1_p, L2_p, L3_p;
    L0_p = __msa_ld_h(Lr_p0 - 1, 0);
    L1_p = __msa_ld_h(Lr_p1 - 1, 0);
    L2_p = __msa_ld_h(Lr_p2 - 1, 0);
    L3_p = __msa_ld_h(Lr_p3 - 1, 0);

    for (d = 0; d < D; d += 16)
      {
	v8i16 L0, L1, L2, L3;
	v8i16 L0_1, L1_1, L2_1, L3_1;
	v8i16 L0_f, L1_f, L2_f, L3_f;
			  
	// v8i16 L0_n, L1_n, L2_n, L3_n;
	// v8i16 L0_n1, L1_n1, L2_n1, L3_n1;
	// v8i16 L0_p1, L1_p1, L2_p1, L3_p1;

	v8i16 Cpd = __msa_ld_h(const_cast<CostType *>(Cp + d), 0);
	v8i16 Cpd1 = __msa_ld_h(const_cast<CostType *>(Cp + d), 16);
	v8i16 L0_n, L1_n, L2_n, L3_n;

	PRELOAD(Lr_p1 + d - 1 + 8, 32);
	L0_p = __msa_ld_h(Lr_p0 + d, -2);
	PRELOAD(Lr_p2 + d - 1 + 8, 32);			  
	L1_p = __msa_ld_h(Lr_p1 + d, -2);
	PRELOAD(Lr_p3 + d - 1 + 8, 32);
	L2_p = __msa_ld_h(Lr_p2 + d, -2);
	L3_p = __msa_ld_h(Lr_p3 + d, -2);
	PRELOAD(Lr_p0 + d - 1 + 8, 32);

	L0   = __msa_ld_h(Lr_p0 + d, 0);
	L1   = __msa_ld_h(Lr_p1 + d, 0);
	L2   = __msa_ld_h(Lr_p2 + d, 0);
	L3   = __msa_ld_h(Lr_p3 + d, 0);
	L0_n = __msa_ld_h(Lr_p0 + d, 2);
	L1_n = __msa_ld_h(Lr_p1 + d, 2);
	L2_n = __msa_ld_h(Lr_p2 + d, 2);
	L3_n = __msa_ld_h(Lr_p3 + d, 2);
	v8i16 L0_n1, L0_p1;
	v8i16 L1_n1, L1_p1;
	v8i16 L2_n1, L2_p1;
	v8i16 L3_n1, L3_p1;
	PRELOAD(Lr_p1 + d - 1 + 16, 32);
	L0_p1 = __msa_ld_h(Lr_p0 + d , 14);
	PRELOAD(Lr_p2 + d - 1 + 16, 32);	
	L1_p1 = __msa_ld_h(Lr_p1 + d , 14);
	PRELOAD(Lr_p3 + d - 1 + 16, 32);
	L2_p1 = __msa_ld_h(Lr_p2 + d , 14);
	L3_p1 = __msa_ld_h(Lr_p3 + d , 14);
	PRELOAD(Lr_p0 + d - 1 + 16, 32);
		
	GET_MIN(L0_p, L0, L0_n, _P1, _delta0, Cpd);
	GET_MIN(L1_p, L1, L1_n, _P1, _delta1, Cpd);
	GET_MIN(L2_p, L2, L2_n, _P1, _delta2, Cpd);
	GET_MIN(L3_p, L3, L3_n, _P1, _delta3, Cpd);
			  

	L0_1  = __msa_ld_h(Lr_p0 + d, 16);
	L1_1  = __msa_ld_h(Lr_p1 + d, 16);
	L2_1  = __msa_ld_h(Lr_p2 + d, 16);
	L3_1  = __msa_ld_h(Lr_p3 + d, 16);	
	L0_n1 = __msa_ld_h(Lr_p0 + d , 18);
	L1_n1 = __msa_ld_h(Lr_p1 + d , 18);
	L2_n1 = __msa_ld_h(Lr_p2 + d , 18);
	L3_n1 = __msa_ld_h(Lr_p3 + d , 18);
			    
			    
	GET_MIN(L0_p1, L0_1, L0_n1, _P1, _delta0, Cpd1);

	GET_MIN(L1_p1, L1_1, L1_n1, _P1, _delta1, Cpd1);


	GET_MIN(L2_p1, L2_1, L2_n1, _P1, _delta2, Cpd1);

	GET_MIN(L3_p1, L3_1, L3_n1, _P1, _delta3, Cpd1);

			  
	__msa_st_h(L0,  Lr_p + d, 0) ;
	__msa_st_h(L0_1,Lr_p + d, 16) ;
	__msa_st_h(L1,  Lr_p + d + D2, 0) ;
	__msa_st_h(L1_1,Lr_p + d + D2, 16) ;
	__msa_st_h(L2,  Lr_p + d + D2*2, 0) ;
	__msa_st_h(L2_1,Lr_p + d + D2*2, 16) ;
	__msa_st_h(L3,  Lr_p + d + D2*3, 0) ;
	__msa_st_h(L3_1,Lr_p + d + D2*3, 16) ;
	PRELOAD(Sp + d, 32);
	_minL0 = __msa_min_s_h(_minL0, L0);
	_minL0 = __msa_min_s_h(_minL0, L0_1);
	_minL1 = __msa_min_s_h(_minL1, L1);
	_minL1 = __msa_min_s_h(_minL1, L1_1);
	_minL2 = __msa_min_s_h(_minL2, L2);
	_minL2 = __msa_min_s_h(_minL2, L2_1);
	_minL3 = __msa_min_s_h(_minL3, L3);
	_minL3 = __msa_min_s_h(_minL3, L3_1);

	v8i16 Sval = __msa_ld_h(Sp + d, 0);
	v8i16 Sval_1 = __msa_ld_h(Sp + d, 16);

	PRESTORE(Sp + d, 0);
	L0 = L0 + L1;
	L2 = L2 + L3;
	Sval = Sval + L0;
	Sval = Sval + L2;
	PRELOAD(Cp + d, 32);
	L0_1 = L0_1 + L1_1;
	L2_1 = L2_1 + L3_1;
	Sval_1 = Sval_1 + L0_1;
	Sval_1 = Sval_1 + L2_1;

	__msa_st_h ( Sval, Sp + d, 0);
	__msa_st_h ( Sval_1, Sp + d, 16);
			  
	// L0_p = L0_f;
	// L1_p = L1_f;
	// L2_p = L2_f;
	// L3_p = L3_f;
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
    // printf("[%d %d ]%d, \n",y, x, minLr[0][xm]);
  }
