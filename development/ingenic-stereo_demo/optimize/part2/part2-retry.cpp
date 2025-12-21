#include <thread>
#include <pthread.h>
#undef GET_MIN
#undef LOAD_Lr
int part2_useSIMD = 1;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

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


class alone_lr
{
public:
  int x1, x2, dx, NR2, D2, P2, NRD2;
  int MAX_COST, P1, D;
  CostType **Lr, *C, *S;
  CostType **minLr;

  alone_lr (int x1, int x2, int dx, int NR2, int D2, int P2, int NRD2, int MAX_COST, int P1, int D,
	    CostType **Lr, CostType* C,CostType *S, CostType **minLr)
    
    : x1(x1), x2(x2), dx(dx), NR2(NR2), D2(D2), P2(P2), NRD2(NRD2), MAX_COST(MAX_COST), P1(P1), D(D),
      Lr(Lr), C(C), S(S), minLr(minLr){}
  void operator()()
  {
    int x, d;
    for ( x = x1; x != x2; x += dx)	     
      {
	int xm = x*NR2, xd = xm*D2;
	int delta1 = minLr[1][xm - NR2 + 1] + P2;
	int delta2 = minLr[1][xm + 2] + P2, delta3 = minLr[1][xm + NR2 + 3] + P2;
		  
	CostType* Lr_p1 = Lr[1] + xd - NRD2 + D2;
	CostType* Lr_p2 = Lr[1] + xd + D2*2;
	CostType* Lr_p3 = Lr[1] + xd + NRD2 + D2*3;
	
	Lr_p1[-1] = Lr_p1[D] =
	  Lr_p2[-1] = Lr_p2[D] = Lr_p3[-1] = Lr_p3[D] = MAX_COST;

	CostType* Lr_p = Lr[0] + xd;
	const CostType* Cp = C + x*D;
	CostType* Sp = S + x*D;
	if (part2_useSIMD)
	  {
	    v8i16 _P1 = __msa_fill_h((short)P1);
	    v8i16 _delta1 = __msa_fill_h((short)delta1);
	    v8i16 _delta2 = __msa_fill_h((short)delta2);
	    v8i16 _delta3 = __msa_fill_h((short)delta3);
	    v8i16 _minL1 = __msa_fill_h((short)MAX_COST);
	    v8i16 _minL2 = __msa_fill_h((short)MAX_COST);
	    v8i16 _minL3 = __msa_fill_h((short)MAX_COST);
	    v8i16 L1, L1_p, L1_n, L1_f;
	    v8i16 L2, L2_p, L2_n, L2_f;
	    v8i16 L3, L3_p, L3_n, L3_f;

	    L1_p = __msa_ld_h(Lr_p1 - 1, 0);
	    L2_p = __msa_ld_h(Lr_p2 - 1, 0);
	    L3_p = __msa_ld_h(Lr_p3 - 1, 0);
	    for (d = 0; d < D; d += 8)
	      {
		v8i16 Cpd = __msa_ld_h(const_cast<CostType *>(Cp + d), 0);
		LOAD_Lr(Lr_p1, L1_f, L1_p, L1, L1_n, d);
		LOAD_Lr(Lr_p2, L2_f, L2_p, L2, L2_n, d);
		LOAD_Lr(Lr_p3, L3_f, L3_p, L3, L3_n, d);
	      
		GET_MIN(L1_p, L1, L1_n, _P1, _delta1, Cpd);
		GET_MIN(L2_p, L2, L2_n, _P1, _delta2, Cpd);
		GET_MIN(L3_p, L3, L3_n, _P1, _delta3, Cpd);
		__msa_st_h(L1, Lr_p + d + D2, 0);
		__msa_st_h(L2, Lr_p + d + D2*2, 0);
		__msa_st_h(L3, Lr_p + d + D2*3, 0);

		_minL1 = __msa_min_s_h(_minL1, L1);
		_minL2 = __msa_min_s_h(_minL2, L2);
		_minL3 = __msa_min_s_h(_minL3, L3);

		pthread_mutex_lock(&mutex);
		v8i16 Sval = __msa_ld_h(Sp + d, 0);
		L2 = L2 + L3;
		Sval = Sval + L2 + L1;
		__msa_st_h ( Sval, Sp + d, 0);
		pthread_mutex_unlock(&mutex);
		L1_p = L1_f;
		L2_p = L2_f;
		L3_p = L3_f;
	      }
	    v8i16 t12l, t12h;

	    t12l = __msa_pckev_h(_minL2, _minL1);
	    t12h = __msa_pckod_h(_minL2, _minL1);
	    v8i16 t12 = __msa_min_s_h(t12h, t12l);
	    v8i16 t123l, t123h;
	    t123l = __msa_pckev_h(_minL3, t12);
	    t123h = __msa_pckod_h(_minL3, t12);
	    v8i16 t123 = __msa_min_s_h(t123l, t123h);
	    v8i16 swap = __msa_shf_h(t123, 0B10110001);
	    t123 = __msa_min_s_h(t123, swap);
	    swap = __msa_shf_w(t123, 0B10110100);
	    t123 = __msa_min_s_h(t123, swap);
	    minLr[0][xm+1] = (CostType)t123[0];
	    minLr[0][xm+2] = (CostType)t123[2];
	    minLr[0][xm+3] = (CostType)t123[4];
	  }
	else
	  {
	    int minL1 = MAX_COST, minL2 = MAX_COST, minL3 = MAX_COST;
	    for( d = 0; d < D; d++ )
	      {
		int Cpd = Cp[d], L1, L2, L3;
	      
		L1 = Cpd + std::min((int)Lr_p1[d],
				    std::min(Lr_p1[d-1] + P1,
					     std::min(Lr_p1[d+1] + P1, delta1))) - delta1;
		L2 = Cpd + std::min((int)Lr_p2[d],
				    std::min(Lr_p2[d-1] + P1,
					     std::min(Lr_p2[d+1] + P1, delta2))) - delta2;
		L3 = Cpd + std::min((int)Lr_p3[d],
				    std::min(Lr_p3[d-1] + P1,
					     std::min(Lr_p3[d+1] + P1, delta3))) - delta3;
		Lr_p[d + D2] = (CostType)L1;	  minL1 = std::min(minL1, L1);
		Lr_p[d + D2*2] = (CostType)L2;  minL2 = std::min(minL2, L2);	  
		Lr_p[d + D2*3] = (CostType)L3;  minL3 = std::min(minL3, L3);
		pthread_mutex_lock(&mutex);
		Sp[d] = saturate_cast<CostType>(Sp[d] + L1 + L2 + L3);
		pthread_mutex_unlock(&mutex);
	      }
	    minLr[0][xm+1] = (CostType)minL1;
	    minLr[0][xm+2] = (CostType)minL2;
	    minLr[0][xm+3] = (CostType)minL3;      
	  }
      } // end for Lr123
  }
};


class dep_lr
{
public:
  int x1, x2, dx, NR2, D2, P2, NRD2;
  int MAX_COST, P1, D;
  CostType **Lr, *C, *S;
  CostType **minLr;
  dep_lr(int x1, int x2, int dx, int NR2, int D2, int P2, int NRD2, int MAX_COST, int P1, int D,
	 CostType **Lr, CostType* C,CostType *S, CostType **minLr)
    
    : x1(x1), x2(x2), dx(dx), NR2(NR2), D2(D2), P2(P2), NRD2(NRD2), MAX_COST(MAX_COST), P1(P1), D(D),
      Lr(Lr), C(C), S(S), minLr(minLr){}
  void operator()()
  {
    int x, d;
    for ( x = x1; x != x2; x += dx)
      {
	int xm = x*NR2, xd = xm*D2;
	int delta0 = minLr[0][xm - dx*NR2] + P2;
	CostType* Lr_p0 = Lr[0] + xd - dx*NRD2;
	Lr_p0[-1] = Lr_p0[D] = MAX_COST;
	CostType* Lr_p = Lr[0] + xd;
	const CostType* Cp = C + x*D;
	CostType* Sp = S + x*D;

	if (part2_useSIMD)
	  {
	    v8i16 _minL0 = __msa_fill_h(MAX_COST);
	    v8i16 _delta0 = __msa_fill_h((short)delta0);
	    v8i16 _P1    = __msa_fill_h((short)P1);
	    v8i16 L0, L0_p, L0_n, L0_f;
	    L0_p = __msa_ld_h(Lr_p0 - 1, 0);

	    for (d = 0; d < D; d += 8)
	      {
		v8i16 Cpd = __msa_ld_h(const_cast<CostType *>(Cp + d), 0);
		LOAD_Lr(Lr_p0, L0_f, L0_p, L0, L0_n, d);
		GET_MIN(L0_p, L0, L0_n, _P1, _delta0, Cpd);
		__msa_st_h(L0, Lr_p + d, 0) ;
		_minL0 = __msa_min_s_h(_minL0, L0);

		pthread_mutex_lock(&mutex);
		v8i16 Sval = __msa_ld_h(Sp + d, 0);
		Sval = Sval + L0;
		__msa_st_h ( Sval, Sp + d, 0);
		pthread_mutex_unlock(&mutex);

		L0_p = L0_f;
	      }
	    v8i16 swap;
	    swap = __msa_shf_h (_minL0, 0B10110001);
	    _minL0 = __msa_min_s_h(_minL0, swap);
	    swap = (v8i16)__msa_shf_w(_minL0, 0B10110001);
	    _minL0 = __msa_min_s_h(_minL0, swap);
	    swap = (v8i16)__msa_shf_w(_minL0, 0B01001110);
	    _minL0 = __msa_min_s_h(_minL0, swap);
	    minLr[0][xm] = _minL0[0];
	  }
	else
	  {
	    int minL0 = MAX_COST;

	    for( d = 0; d < D; d++ )
	      {
		int Cpd = Cp[d], L0;
		L0 = Cpd + std::min((int)Lr_p0[d], std::min(Lr_p0[d-1] + P1, std::min(Lr_p0[d+1] + P1, delta0))) - delta0;
		Lr_p[d] = (CostType)L0;
		minL0 = std::min(minL0, L0);
		pthread_mutex_lock(&mutex);
		Sp[d] = saturate_cast<CostType>(Sp[d] + L0);
		pthread_mutex_unlock(&mutex);
	      }
	    minLr[0][xm] = (CostType)minL0;
	  }
      } // end for Lr0
  }
};
#undef PRELOAD
#define PRELOAD(base,off) asm volatile ("pref 0,%1(%0)\n" ::"r"(base),"i"(off):"memory")

static inline void part2(int x1, int x2, int dx, int NR2, int D2, int P2, int NRD2, int MAX_COST, int P1, int D,
			 CostType **Lr, CostType* C,CostType *S, CostType **minLr)
{
  if(1)
    {
      alone_lr al( x1, x2,  dx,  NR2,  D2,  P2,  NRD2, MAX_COST, P1, D, Lr, C, S, minLr);
      dep_lr dl( x1, x2,  dx,  NR2,  D2,  P2,  NRD2, MAX_COST, P1, D, Lr, C, S, minLr);
      thread thread1(al);
      thread thread2(dl);
      thread1.join();
      thread2.join();
    }
  else
    {
      int x, d;
      for ( x = x1; x != x2; x += dx)
	{
	  int xm = x*NR2, xd = xm*D2;
	  int delta0 = minLr[0][xm - dx*NR2] + P2;
	  CostType* Lr_p0 = Lr[0] + xd - dx*NRD2;
	  Lr_p0[-1] = Lr_p0[D] = MAX_COST;
	  CostType* Lr_p = Lr[0] + xd;
	  const CostType* Cp = C + x*D;
	  CostType* Sp = S + x*D;

	  if (1)
	    {
	      asm ("ssnop \n\t");
	      asm ("ssnop \n\t");
	      asm ("ssnop \n\t");

	      v8i16 _minL0 = __msa_fill_h(MAX_COST);
	      v8i16 _delta0 = __msa_fill_h((short)delta0);
	      v8i16 _P1    = __msa_fill_h((short)P1);
	      v8i16 L0, L0_p, L0_n, L0_f;
	      L0_p = __msa_ld_h(Lr_p0 - 1, 0);

	      for (d = 0; d < D; d += 16)
		{
		  v8i16 L0;
		  v8i16 L0_1;
		  v8i16 L0_f;

		  v8i16 Cpd = __msa_ld_h(const_cast<CostType *>(Cp + d), 0);
		  v8i16 Cpd1 = __msa_ld_h(const_cast<CostType *>(Cp + d), 16);
		  {
		    v8i16 L0_n, L0_n1, L0_p1;			 
		    L0_p1 = __msa_ld_h(Lr_p0 + d - 1, 16);
		    L0_f  = __msa_ld_h(Lr_p0 + d - 1, 32);
		    L0    = __msa_vsldi_b(L0_p, L0_p1, 2);
		    L0_n  = __msa_vsldi_b(L0_p, L0_p1, 4);
		    L0_1  = __msa_vsldi_b(L0_p1, L0_f, 2);
		    L0_n1 = __msa_vsldi_b(L0_p1, L0_f, 4);
		    GET_MIN(L0_p, L0, L0_n, _P1, _delta0, Cpd);
		    GET_MIN(L0_p1, L0_1, L0_n1, _P1, _delta0, Cpd1);
		  }

		  __msa_st_h(L0,  Lr_p + d, 0) ;
		  __msa_st_h(L0_1,Lr_p + d, 16) ;
			  
		  _minL0 = __msa_min_s_h(_minL0, L0);
		  _minL0 = __msa_min_s_h(_minL0, L0_1);


		  v8i16 Sval = __msa_ld_h(Sp + d, 0);
		  v8i16 Sval_1 = __msa_ld_h(Sp + d, 16);
		  Sval = Sval + L0;
		  Sval_1 = Sval_1 + L0_1;
		  __msa_st_h ( Sval, Sp + d, 0);
		  __msa_st_h ( Sval_1, Sp + d, 16);

		  L0_p = L0_f;
		}
	      v8i16 swap;
	      swap = __msa_shf_h (_minL0, 0B10110001);
	      _minL0 = __msa_min_s_h(_minL0, swap);
	      swap = (v8i16)__msa_shf_w(_minL0, 0B10110001);
	      _minL0 = __msa_min_s_h(_minL0, swap);
	      swap = (v8i16)__msa_shf_w(_minL0, 0B01001110);
	      _minL0 = __msa_min_s_h(_minL0, swap);
	      minLr[0][xm] = _minL0[0];
	      	      asm ("ssnop \n\t");
	      asm ("ssnop \n\t");
	      asm ("ssnop \n\t");

	    }
	  else
	    {
	      int minL0 = MAX_COST;

	      for( d = 0; d < D; d++ )
		{
		  int Cpd = Cp[d], L0;
		  L0 = Cpd + std::min((int)Lr_p0[d], std::min(Lr_p0[d-1] + P1, std::min(Lr_p0[d+1] + P1, delta0))) - delta0;
		  Lr_p[d] = (CostType)L0;
		  minL0 = std::min(minL0, L0);
		  Sp[d] = saturate_cast<CostType>(Sp[d] + L0);	    
		}
	      minLr[0][xm] = (CostType)minL0;
	    }
	} // end for Lr0

      for ( x = x1; x != x2; x += dx)	     
	{
	  int xm = x*NR2, xd = xm*D2;
	  int delta1 = minLr[1][xm - NR2 + 1] + P2;
	  int delta2 = minLr[1][xm + 2] + P2, delta3 = minLr[1][xm + NR2 + 3] + P2;
		  
	  CostType* Lr_p1 = Lr[1] + xd - NRD2 + D2;
	  CostType* Lr_p2 = Lr[1] + xd + D2*2;
	  CostType* Lr_p3 = Lr[1] + xd + NRD2 + D2*3;
	
	  Lr_p1[-1] = Lr_p1[D] =
	    Lr_p2[-1] = Lr_p2[D] = Lr_p3[-1] = Lr_p3[D] = MAX_COST;

	  CostType* Lr_p = Lr[0] + xd;
	  const CostType* Cp = C + x*D;
	  CostType* Sp = S + x*D;
	  if (1)
	    {
	      asm ("ssnop \n\t");
	      asm ("ssnop \n\t");
	      asm ("ssnop \n\t");
	      v8i16 _P1 = __msa_fill_h((short)P1);
	      v8i16 _delta1 = __msa_fill_h((short)delta1);
	      v8i16 _delta2 = __msa_fill_h((short)delta2);
	      v8i16 _delta3 = __msa_fill_h((short)delta3);
	      v8i16 _minL1 = __msa_fill_h((short)MAX_COST);
	      v8i16 _minL2 = __msa_fill_h((short)MAX_COST);
	      v8i16 _minL3 = __msa_fill_h((short)MAX_COST);
	      v8i16 L0_p, L1_p, L2_p, L3_p;
	      L1_p = __msa_ld_h(Lr_p1 - 1, 0);
	      L2_p = __msa_ld_h(Lr_p2 - 1, 0);
	      L3_p = __msa_ld_h(Lr_p3 - 1, 0);
	      
	      for (d = 0; d < D; d += 16)
		{
		  v8i16 L0, L1, L2, L3;
		  v8i16 L0_1, L1_1, L2_1, L3_1;
		  v8i16 L0_f, L1_f, L2_f, L3_f;

		  v8i16 Cpd = __msa_ld_h(const_cast<CostType *>(Cp + d), 0);
		  v8i16 Cpd1 = __msa_ld_h(const_cast<CostType *>(Cp + d), 16);
 
		  {
		    v8i16 L1_n, L1_n1, L1_p1;
		    L1_p1 = __msa_ld_h(Lr_p1 + d - 1, 16);
		    L1_f	= __msa_ld_h(Lr_p1 + d - 1, 32);

		    PRELOAD(Lr_p1 + d - 1 + 16, 0);
				    
		    L1	= __msa_vsldi_b(L1_p, L1_p1, 2);
		    L1_n	= __msa_vsldi_b(L1_p, L1_p1, 4);
		    L1_1	= __msa_vsldi_b(L1_p1,L1_f, 2);
		    L1_n1 = __msa_vsldi_b(L1_p1,L1_f, 4);

		    GET_MIN(L1_p, L1, L1_n, _P1, _delta1, Cpd);
		    GET_MIN(L1_p1, L1_1, L1_n1, _P1, _delta1, Cpd1);
		  }
		  {
		    v8i16 L2_n, L2_n1, L2_p1;
		    L2_p1 = __msa_ld_h(Lr_p2 + d - 1, 16);
		    L2_f	= __msa_ld_h(Lr_p2 + d - 1, 32);

		    PRELOAD(Lr_p2 + d - 1 + 16, 0);
		    
		    L2	= __msa_vsldi_b(L2_p, L2_p1, 2);
		    L2_n	= __msa_vsldi_b(L2_p, L2_p1, 4);
		    L2_1	= __msa_vsldi_b(L2_p1,L2_f, 2);
		    L2_n1 = __msa_vsldi_b(L2_p1,L2_f, 4);

		    GET_MIN(L2_p, L2, L2_n, _P1, _delta2, Cpd);
		    GET_MIN(L2_p1, L2_1, L2_n1, _P1, _delta2, Cpd1);
		  }
		  {
		    v8i16 L3_n, L3_n1, L3_p1;
		    L3_p1 = __msa_ld_h(Lr_p3 + d - 1, 16);
		    L3_f	= __msa_ld_h(Lr_p3 + d - 1, 32);

		    PRELOAD(Lr_p3 + d - 1 + 16, 0);
		    
		    L3	= __msa_vsldi_b(L3_p, L3_p1, 2);
		    L3_n	= __msa_vsldi_b(L3_p, L3_p1, 4);
		    L3_1	= __msa_vsldi_b(L3_p1,L3_f, 2);
		    L3_n1 = __msa_vsldi_b(L3_p1,L3_f, 4);
		    GET_MIN(L3_p, L3, L3_n, _P1, _delta3, Cpd);
		    GET_MIN(L3_p1, L3_1, L3_n1, _P1, _delta3, Cpd1);
		  }
		  PRELOAD(Sp + d, 16);
		  __msa_st_h(L1,  Lr_p + d + D2, 0) ;
		  __msa_st_h(L1_1,Lr_p + d + D2, 16) ;
		  __msa_st_h(L2,  Lr_p + d + D2*2, 0) ;
		  __msa_st_h(L2_1,Lr_p + d + D2*2, 16) ;
		  __msa_st_h(L3,  Lr_p + d + D2*3, 0) ;
		  __msa_st_h(L3_1,Lr_p + d + D2*3, 16) ;
		  _minL1 = __msa_min_s_h(_minL1, L1);
		  _minL1 = __msa_min_s_h(_minL1, L1_1);
		  _minL2 = __msa_min_s_h(_minL2, L2);
		  _minL2 = __msa_min_s_h(_minL2, L2_1);
		  _minL3 = __msa_min_s_h(_minL3, L3);
		  _minL3 = __msa_min_s_h(_minL3, L3_1);
		  
		  v8i16 Sval = __msa_ld_h(Sp + d, 0);
		  L2 = L2 + L3;
		  Sval = Sval + L2 + L1;
		  v8i16 Sval_1 = __msa_ld_h(Sp + d, 16);
		  L2_1 = L2_1 + L3_1;
		  Sval_1 = Sval_1 + L2_1 + L1_1;
		  
		  __msa_st_h ( Sval, Sp + d, 0);
		  __msa_st_h ( Sval_1, Sp + d, 16);
		  L1_p = L1_f;
		  L2_p = L2_f;
		  L3_p = L3_f;
		}
	      v8i16 t12l, t12h;

	      t12l = __msa_pckev_h(_minL2, _minL1);
	      t12h = __msa_pckod_h(_minL2, _minL1);
	      v8i16 t12 = __msa_min_s_h(t12h, t12l);
	      v8i16 t123l, t123h;
	      t123l = __msa_pckev_h(_minL3, t12);
	      t123h = __msa_pckod_h(_minL3, t12);
	      v8i16 t123 = __msa_min_s_h(t123l, t123h);
	      v8i16 swap = __msa_shf_h(t123, 0B10110001);
	      t123 = __msa_min_s_h(t123, swap);
	      swap = __msa_shf_w(t123, 0B10110100);
	      t123 = __msa_min_s_h(t123, swap);
	      minLr[0][xm+1] = (CostType)t123[0];
	      minLr[0][xm+2] = (CostType)t123[2];
	      minLr[0][xm+3] = (CostType)t123[4];
	      asm ("ssnop \n\t");
	      asm ("ssnop \n\t");
	      asm ("ssnop \n\t");
	      
	    }
	  else
	    {
	      int minL1 = MAX_COST, minL2 = MAX_COST, minL3 = MAX_COST;
	      for( d = 0; d < D; d++ )
		{
		  int Cpd = Cp[d], L1, L2, L3;
	      
		  L1 = Cpd + std::min((int)Lr_p1[d],
				      std::min(Lr_p1[d-1] + P1,
					       std::min(Lr_p1[d+1] + P1, delta1))) - delta1;
		  L2 = Cpd + std::min((int)Lr_p2[d],
				      std::min(Lr_p2[d-1] + P1,
					       std::min(Lr_p2[d+1] + P1, delta2))) - delta2;
		  L3 = Cpd + std::min((int)Lr_p3[d],
				      std::min(Lr_p3[d-1] + P1,
					       std::min(Lr_p3[d+1] + P1, delta3))) - delta3;
		  Lr_p[d + D2] = (CostType)L1;	  minL1 = std::min(minL1, L1);
		  Lr_p[d + D2*2] = (CostType)L2;  minL2 = std::min(minL2, L2);	  
		  Lr_p[d + D2*3] = (CostType)L3;  minL3 = std::min(minL3, L3);
		  Sp[d] = saturate_cast<CostType>(Sp[d] + L1 + L2 + L3);
		}
	      minLr[0][xm+1] = (CostType)minL1;
	      minLr[0][xm+2] = (CostType)minL2;
	      minLr[0][xm+3] = (CostType)minL3;      
	    }
	} // end for Lr123
    }
}

#undef GET_MIN
#undef LOAD_Lr
