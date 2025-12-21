
#if !PART0_OP
static void calcPixelCostBT( const IMat& img1, const IMat& img2, int y,
			     int minD, int maxD, CostType* cost,
			     PixType* buffer, const PixType* tab,
			     int tabOfs, int , int xrange_min = 0,
			     int xrange_max = DEFAULT_RIGHT_BORDER )
{
  int x, c, width = img1.cols, cn = img1.channels();
  int minX1 = std::max(maxD, 0), maxX1 = width + std::min(minD, 0);
  int D = maxD - minD, width1 = maxX1 - minX1;
  //This minX1 & maxX2 correction is defining which part of calculatable line must be calculated
  //That is needs of parallel algorithm
  xrange_min = (xrange_min < 0) ? 0: xrange_min;
  xrange_max = (xrange_max == DEFAULT_RIGHT_BORDER) || (xrange_max > width1) ? width1 : xrange_max;
  maxX1 = minX1 + xrange_max;
  minX1 += xrange_min;
  width1 = maxX1 - minX1;
  int minX2 = std::max(minX1 - maxD, 0), maxX2 = std::min(maxX1 - minD, width);
  int width2 = maxX2 - minX2;
  const PixType *row1 = img1.ptr<PixType>(y), *row2 = img2.ptr<PixType>(y);
  PixType *prow1 = buffer + width2*2, *prow2 = prow1 + width*cn*2;
  tab += tabOfs;

  for( c = 0; c < cn*2; c++ )
    {
      prow1[width*c] = prow1[width*c + width-1] =
        prow2[width*c] = prow2[width*c + width-1] = tab[0];
    }

  int n1 = y > 0 ? -(int)img1.stride : 0, s1 = y < img1.rows-1 ? (int)img1.stride : 0;
  int n2 = y > 0 ? -(int)img2.stride : 0, s2 = y < img2.rows-1 ? (int)img2.stride : 0;
  
  int minX_cmn = std::min(minX1,minX2)-1;
  int maxX_cmn = std::max(maxX1,maxX2)+1;
  minX_cmn = std::max(minX_cmn, 1);
  maxX_cmn = std::min(maxX_cmn, width - 1);
  if( cn == 1 )
    {
      for( x = minX_cmn; x < maxX_cmn; x++ )
        {
	  prow1[x] = tab[(row1[x+1] - row1[x-1])*2 + row1[x+n1+1] - row1[x+n1-1] + row1[x+s1+1] - row1[x+s1-1]];
	  prow2[width-1-x] = tab[(row2[x+1] - row2[x-1])*2 + row2[x+n2+1] - row2[x+n2-1] + row2[x+s2+1] - row2[x+s2-1]];

	  prow1[x+width] = row1[x];
	  prow2[width-1-x+width] = row2[x];
        }
    }
  else
    {
      // printf("invalid channel for get cost\n");
      for( x = minX_cmn; x < maxX_cmn; x++ )
        {
	  prow1[x] = tab[(row1[x*3+3] - row1[x*3-3])*2 + row1[x*3+n1+3] - row1[x*3+n1-3] + row1[x*3+s1+3] - row1[x*3+s1-3]];
	  prow1[x+width] = tab[(row1[x*3+4] - row1[x*3-2])*2 + row1[x*3+n1+4] - row1[x*3+n1-2] + row1[x*3+s1+4] - row1[x*3+s1-2]];
	  prow1[x+width*2] = tab[(row1[x*3+5] - row1[x*3-1])*2 + row1[x*3+n1+5] - row1[x*3+n1-1] + row1[x*3+s1+5] - row1[x*3+s1-1]];

	  prow2[width-1-x] = tab[(row2[x*3+3] - row2[x*3-3])*2 + row2[x*3+n2+3] - row2[x*3+n2-3] + row2[x*3+s2+3] - row2[x*3+s2-3]];
	  prow2[width-1-x+width] = tab[(row2[x*3+4] - row2[x*3-2])*2 + row2[x*3+n2+4] - row2[x*3+n2-2] + row2[x*3+s2+4] - row2[x*3+s2-2]];
	  prow2[width-1-x+width*2] = tab[(row2[x*3+5] - row2[x*3-1])*2 + row2[x*3+n2+5] - row2[x*3+n2-1] + row2[x*3+s2+5] - row2[x*3+s2-1]];

	  prow1[x+width*3] = row1[x*3];
	  prow1[x+width*4] = row1[x*3+1];
	  prow1[x+width*5] = row1[x*3+2];

	  prow2[width-1-x+width*3] = row2[x*3];
	  prow2[width-1-x+width*4] = row2[x*3+1];
	  prow2[width-1-x+width*5] = row2[x*3+2];
        }
    }
  memset( cost + xrange_min*D, 0, width1*D*sizeof(cost[0]) );

  buffer -= width-1-maxX2;
  cost -= (minX1-xrange_min)*D + minD; // simplify the cost indices inside the loop

  for( c = 0; c < cn*2; c++, prow1 += width, prow2 += width )
    {
      int diff_scale = c < cn ? 0 : 2;
      for( x = width-1-maxX2; x < width-1- minX2; x++ )
        {
	  int v = prow2[x];
	  int vl = x > 0 ? (v + prow2[x-1])/2 : v;
	  int vr = x < width-1 ? (v + prow2[x+1])/2 : v;
	  int v0 = std::min(vl, vr); v0 = std::min(v0, v);
	  int v1 = std::max(vl, vr); v1 = std::max(v1, v);
	  buffer[x] = (PixType)v0;
	  buffer[x + width2] = (PixType)v1;
        }

      for( x = minX1; x < maxX1; x++ )
        {
	  int u = prow1[x];
	  int ul = x > 0 ? (u + prow1[x-1])/2 : u;
	  int ur = x < width-1 ? (u + prow1[x+1])/2 : u;
	  int u0 = std::min(ul, ur); u0 = std::min(u0, u);
	  int u1 = std::max(ul, ur); u1 = std::max(u1, u);
	  {
	    for( int d = minD; d < maxD; d++ )
	      {
		int v = prow2[width-x-1 + d];
		int v0 = buffer[width-x-1 + d];
		int v1 = buffer[width-x-1 + d + width2];
		int c0 = std::max(0, u - v1); c0 = std::max(c0, v0 - u);
		int c1 = std::max(0, v - u1); c1 = std::max(c1, u0 - v);

		cost[x*D + d] = (CostType)(cost[x*D+d] + (std::min(c0, c1) >> diff_scale));
	      }
	  }
        }
    }
}

#else

#define v_average(v,d) ({			\
      v16u8 av;					\
      v8u16 add[2];				\
      add[0] =__msa_addur_h(v,d);		\
      add[1] =__msa_addul_h(v,d);		\
      add[0] = add[0] >> 1;			\
      add[1] = add[1] >> 1;			\
      av = __msa_pckev_b(add[1],add[0]);	\
      av;					\
    })

#define V_ZERO_COST(index) do{			\
    v_cost[0] = __msa_ldi_b(0);			\
    v_cost[1] = v_cost[0];			\
  }while(0)

#define V_ST_COST(index) do{				\
    __msa_st_b(v_cost[0],&cost[(x +index)*D+d],0);	\
    __msa_st_b(v_cost[1],&cost[(x +index)*D+d],16);	\
  }while(0)

#define V_LD_COST(index) do{				\
    v_cost[0] = __msa_ld_b(&cost[(x +index)*D+d],0);	\
    v_cost[1] = __msa_ld_b(&cost[(x +index)*D+d],16);	\
  }while(0)


#define V_COST_CAL(index,shift) do{			\
    v16u8 v_v,v_v0,v_v1;				\
    v_v = __msa_vsldi_b(v_prow2_x,v_prow2_d,15);	\
    v_v0 = __msa_vsldi_b(v_buffer0_x,v_buffer0_d,15);	\
    v_v1 = __msa_vsldi_b(v_buffer1_x,v_buffer1_d,15);	\
    v_prow2_d = v_v;					\
    v_buffer0_d = v_v0;					\
    v_buffer1_d = v_v1;					\
    v_prow2_x = __msa_slli_v(v_prow2_x,8);		\
    v_buffer0_x = __msa_slli_v(v_buffer0_x,8);		\
    v_buffer1_x = __msa_slli_v(v_buffer1_x,8);		\
    v16u8 v_uu = __msa_splat_b(v_u,index);		\
    v16u8 v_c0 = __msa_subs_u_b(v_uu,v_v1);		\
    v16u8 v_c = __msa_subs_u_b(v_v0,v_uu);		\
    v_c0 = __msa_max_u_b(v_c0,v_c);			\
    v16u8 v_uu0 = __msa_splat_b(v_u0,index);		\
    v16u8 v_uu1 = __msa_splat_b(v_u1,index);		\
    v16u8 v_c1 = __msa_subs_u_b(v_v,v_uu1);		\
    v_c = __msa_subs_u_b(v_uu0,v_v);			\
    v_c1 = __msa_max_u_b(v_c1,v_c);			\
    v_c = __msa_min_u_b(v_c0,v_c1);			\
    if(shift != 0)					\
      v_c = v_c >> shift;				\
    v_cost[0] = __msa_accsr_h(v_cost[0],v_c);		\
    v_cost[1] = __msa_accsl_h(v_cost[1],v_c);		\
  }while(0)


static inline void cal_prow2buffer(int minX2,int maxX2,PixType *prow2,PixType *buffer,int width,int width2)
{
  int x;
  int st = width-1-maxX2;
  int en = width-1- minX2;
  x = st;
  if(st < 0){
    for(x = st;x < 0;x++){
      int v = prow2[x];
      int vl = x > 0 ? (v + prow2[x-1])/2 : v;
      int vr = x < width-1 ? (v + prow2[x+1])/2 : v;
      int v0 = std::min(vl, vr); v0 = std::min(v0, v);
      int v1 = std::max(vl, vr); v1 = std::max(v1, v);
      buffer[x] = (PixType)v0;
      buffer[x + width2] = (PixType)v1;
    }
    st = 0;
  }
  if(en >= width - 1)
    en = width - 1;

  for( x = st; x < (en - st) / 16 * 16; x+= 16)
    {
      v16u8 v_prow2_0 = __msa_ld_b(&prow2[x-1],0);
      v16u8 v_tmp = __msa_ldi_b(0);
      v_tmp = __msa_insert_b(v_tmp,0,prow2[x + 15]);
      v_tmp = __msa_insert_b(v_tmp,1,prow2[x + 16]);
      v16u8 v_prow2 = __msa_vsldi_b(v_prow2_0,v_tmp,1);
      v16u8 v_prow2_1 = __msa_vsldi_b(v_prow2_0,v_tmp,2);

      v16u8 v_vl;
      v16u8 v_vr;

      v_vl = v_average(v_prow2_0,v_prow2);
      v_vr = v_average(v_prow2_1,v_prow2);
      v16u8 v_v;
      v16u8 v_v0;
      v16u8 v_v1;
      v_v = v_prow2;
      v_v0 = __msa_min_u_b(v_vl,v_vr);
      v_v0 = __msa_min_u_b(v_v,v_v0);
      v_v1 = __msa_max_u_b(v_vl,v_vr);
      v_v1 = __msa_max_u_b(v_v,v_v1);

      __msa_st_b(v_v0,&buffer[x],0);
      __msa_st_b(v_v1,&buffer[x + width2],0);
    }
  for(;x < width - 1;x++){
    int v = prow2[x];
    int vl = x > 0 ? (v + prow2[x-1])/2 : v;
    int vr = x < width-1 ? (v + prow2[x+1])/2 : v;
    int v0 = std::min(vl, vr); v0 = std::min(v0, v);
    int v1 = std::max(vl, vr); v1 = std::max(v1, v);
    buffer[x] = (PixType)v0;
    buffer[x + width2] = (PixType)v1;
  }
}
template<int diff_scale>
static inline void cal_cost(PixType *prow1,PixType *prow2,PixType *buffer,int minX1,int maxX1,int minD,int maxD,int width,int width2,CostType *cost,int D)
{
  int x;
  if(minX1 > 0){
    x = minX1;
    if(maxX1 >= width-1)
      {
	for(;x < minX1 + ((width - 1) - minX1) / 16 * 16;x+=16){
	  v16u8 v_prow0 = __msa_ld_b(prow1 + x - 1,0);
	  v16u8 v_tmp = __msa_ldi_b(0);
	  int a = prow1[x + 15] | prow1[x + 16] << 8;
	  v_tmp = __msa_insert_h(v_tmp,0,a);
	  v16u8 v_prow = __msa_vsldi_b(v_prow0,v_tmp,1);
	  v16u8 v_prow1 = __msa_vsldi_b(v_prow0,v_tmp,2);

	  v16u8 v_ul;
	  v16u8 v_ur;

	  v_ul = v_average(v_prow0,v_prow);
	  v_ur = v_average(v_prow1,v_prow);

	  v16u8 v_u;
	  v16u8 v_u0;
	  v16u8 v_u1;
	  v_u = v_prow;
	  v_u0 = __msa_min_u_b(v_ul,v_ur);
	  v_u0 = __msa_min_u_b(v_u,v_u0);
	  v_u1 = __msa_max_u_b(v_ul,v_ur);
	  v_u1 = __msa_max_u_b(v_u,v_u1);
	  int d;
	  for(d = minD;d < (maxD - minD) / 16 * 16 + minD;d+=16){
	    v16u8 v_prow2_x = __msa_ld_b(&prow2[width-x-1 - 15 + d],0);
	    v16u8 v_prow2_d = __msa_ld_b(&prow2[width-x-1 - 15 + d],16);
	    v16u8 v_buffer0_x = __msa_ld_b(&buffer[width-x-1 - 15 + d],0);
	    v16u8 v_buffer0_d = __msa_ld_b(&buffer[width-x-1 - 15 + d],16);

	    v16u8 v_buffer1_x = __msa_ld_b(&buffer[width-x-1 - 15 + d + width2],0);
	    v16u8 v_buffer1_d = __msa_ld_b(&buffer[width-x-1 - 15 + d + width2],16);
	    v8i16 v_cost[2];

#define CAL_COST(index) do{			\
	      if(diff_scale == 0)		\
		V_ZERO_COST(index);		\
	      else				\
		V_LD_COST(index);		\
	      V_COST_CAL(index,diff_scale);	\
	      V_ST_COST(index);			\
	    }while(0)
	    for(int i = 0;i < 16;i+=2){
	      CAL_COST(i + 0);
	      CAL_COST(i + 1);
	    }

#undef CAL_COST
	  }
	  if(d < maxD){
	    int count = maxD - d;
	    d = maxD - 16;
	    v16u8 v_prow2_x = __msa_ld_b(&prow2[width-x-1 - 15 + d],0);
	    v16u8 v_prow2_d = __msa_ld_b(&prow2[width-x-1 - 15 + d],16);
	    v16u8 v_buffer0_x = __msa_ld_b(&buffer[width-x-1 - 15 + d],0);
	    v16u8 v_buffer0_d = __msa_ld_b(&buffer[width-x-1 - 15 + d],16);

	    v16u8 v_buffer1_x = __msa_ld_b(&buffer[width-x-1 - 15 + d + width2],0);
	    v16u8 v_buffer1_d = __msa_ld_b(&buffer[width-x-1 - 15 + d + width2],16);
	    v8i16 v_cost[2];

	    int pos = 16 - count;

#define V_ST_COST_SPARE(index) do{					\
	      if(count > 8){						\
		if(pos == 0)						\
		  __msa_st_b(v_cost[0],&cost[(x + index)*D+d],0);	\
		else{							\
									\
		  for(int i = 0;i < count - 8;i++)			\
		    {							\
		      cost[(x+index) * D + d + pos + i] = v_cost[0][pos + i]; \
		    }							\
		}							\
		__msa_st_b(v_cost[1],&cost[(x + index)*D+d],16);	\
	      }else{							\
		for(int i = 0;i < count;i++)				\
		  {							\
		    cost[(x + index) *D + d + pos + i] = v_cost[1][pos + i - 8]; \
		  }							\
	      }								\
	    }while(0)

#define CAL_COST(index) do{			\
	    if(diff_scale == 0)			\
	      V_ZERO_COST(index);		\
	    else				\
	      V_LD_COST(index);			\
	    V_COST_CAL(index,diff_scale);	\
	    V_ST_COST_SPARE(index);		\
	  }while(0)

	    for(int i = 0;i < 16;i+=2){
	    CAL_COST(i + 0);
	    CAL_COST(i + 1);
	  }
#undef CAL_COST
	  }
	  }
	    //printf("\n");

	  }
	    for(; x < maxX1; x++ )
	      {
	    int u = prow1[x];
	    int ul = x > 0 ? (u + prow1[x-1])/2 : u;
	    int ur = x < width-1 ? (u + prow1[x+1])/2 : u;
	    int u0 = std::min(ul, ur); u0 = std::min(u0, u);
	    int u1 = std::max(ul, ur); u1 = std::max(u1, u);
	    v16u8 v_u = __msa_fill_b(u);
	    v16u8 v_u0 = __msa_fill_b(u0);
	    v16u8 v_u1 = __msa_fill_b(u1);

	    {
	    int d;
	    for(d = minD;d < (maxD - minD) / 16 * 16 + minD;d+=16){
	    v16u8 v_v = __msa_ld_b(&prow2[width-x-1 + d],0);
	    v16u8 v_v0 = __msa_ld_b(&buffer[width-x-1 + d],0);
	    v16u8 v_v1 = __msa_ld_b(&buffer[width-x-1 + d + width2],0);
	    v8i16 v_cost[2];

	    v16u8 v_c0 = __msa_subs_u_b(v_u,v_v1);
	    v16u8 v_c = __msa_subs_u_b(v_v0,v_u);
	    v_c0 = __msa_max_u_b(v_c0,v_c);
	    v16u8 v_c1 = __msa_subs_u_b(v_v,v_u1);
	    v_c = __msa_subs_u_b(v_u0,v_v);
	    v_c1 = __msa_max_u_b(v_c1,v_c);
	    v_c = __msa_min_u_b(v_c0,v_c1);
	    v_c = v_c >> diff_scale;
	    if(diff_scale != 0){
	    v_cost[0] = __msa_ld_b(&cost[x*D+d],0);
	    v_cost[1] = __msa_ld_b(&cost[x*D+d],16);
	  }else{
	    v_cost[0] = __msa_ldi_b(0);
	    v_cost[1] = v_cost[0];
	  }
	    v_cost[0] = __msa_accsr_h(v_cost[0],v_c);
	    v_cost[1] = __msa_accsl_h(v_cost[1],v_c);

	    __msa_st_b(v_cost[0],&cost[x*D+d],0);
	    __msa_st_b(v_cost[1],&cost[x*D+d],16);

	  }
	    for(; d < maxD; d++ )
	      {
	    int v = prow2[width-x-1 + d];
	    int v0 = buffer[width-x-1 + d];
	    int v1 = buffer[width-x-1 + d + width2];
	    int c0 = std::max(0, u - v1); c0 = std::max(c0, v0 - u);
	    int c1 = std::max(0, v - u1); c1 = std::max(c1, u0 - v);
	    if(diff_scale == 0){
	    cost[x*D + d] = std::min(c0, c1);
	  }else
	      cost[x*D + d] = (CostType)(cost[x*D+d] + (std::min(c0, c1) >> diff_scale));
	  }
	  }
	  }
	  }else
	      {
	    for( x = minX1; x < maxX1; x++ )
	      {
	    int u = prow1[x];
	    int ul = x > 0 ? (u + prow1[x-1])/2 : u;
	    int ur = x < width-1 ? (u + prow1[x+1])/2 : u;
	    int u0 = std::min(ul, ur); u0 = std::min(u0, u);
	    int u1 = std::max(ul, ur); u1 = std::max(u1, u);
	    {
	    for( int d = minD; d < maxD; d++ )
	      {
	    int v = prow2[width-x-1 + d];
	    int v0 = buffer[width-x-1 + d];
	    int v1 = buffer[width-x-1 + d + width2];
	    int c0 = std::max(0, u - v1); c0 = std::max(c0, v0 - u);
	    int c1 = std::max(0, v - u1); c1 = std::max(c1, u0 - v);
	    if(diff_scale == 0){
	    cost[x*D + d] = std::min(c0, c1);
	  }else
	      cost[x*D + d] = (CostType)(cost[x*D+d] + (std::min(c0, c1) >> diff_scale));
	  }
	  }

	  }
	  }
	  }

#define LD_ROW(x,row,v_row) do{						\
	    v_row[0] = __msa_ld_b((void*)&row[x - 1],0);		\
	    unsigned int d = row[x - 1 + 16] | (row[x - 1 + 17] << 8);	\
	    v16i8 v_tmp = __msa_ldi_b(0);				\
	    v_tmp = __msa_insert_h(v_tmp,0,d);				\
	    v_row[1] = __msa_vsldi_b(v_row[0],v_tmp,1);			\
	    v_row[2] = __msa_vsldi_b(v_row[0],v_tmp,2);			\
	  }while(0)


#define CAL_INDEX(v_row,v_row_n,v_row_s) do{			\
	    v8i16 v_sub[2];					\
	    v_sub[0] = __msa_subur_h(v_row[2],v_row[0]);	\
	    v_sub[1] = __msa_subul_h(v_row[2],v_row[0]);	\
	    v_index1[0] = v_sub[0] << 1;			\
	    v_index1[1] = v_sub[1] << 1;			\
								\
	    v_sub[0] = __msa_subur_h(v_row_n[2],v_row_n[0]);	\
	    v_sub[1] = __msa_subul_h(v_row_n[2],v_row_n[0]);	\
								\
	    v_index1[0] += v_sub[0];				\
	    v_index1[1] += v_sub[1];				\
								\
	    v_sub[0] = __msa_subur_h(v_row_s[2],v_row_s[0]);	\
	    v_sub[1] = __msa_subul_h(v_row_s[2],v_row_s[0]);	\
	    v_index1[0] += v_sub[0];				\
	    v_index1[1] += v_sub[1];				\
	  }while(0)

#define ST_PROW(prow,xx,i0,i1) do{				\
	    int index;						\
	    index = __msa_copy_s_h(v_index1[i0 >> 3],(i0 & 7));	\
	    prow[xx + i1] = tab[index];				\
	  }while(0)

	    static void calcPixelCostBT( const IMat& img1, const IMat& img2, int y,
	      int minD, int maxD, CostType* cost,
	      PixType* buffer, const PixType* tab,
	      int tabOfs, int , int xrange_min = 0,
	      int xrange_max = DEFAULT_RIGHT_BORDER )
	    {
	    int x, c, width = img1.cols, cn = img1.channels();
	    int minX1 = std::max(maxD, 0), maxX1 = width + std::min(minD, 0);
	    int D = maxD - minD, width1 = maxX1 - minX1;
	    //This minX1 & maxX2 correction is defining which part of calculatable line must be calculated
	    //That is needs of parallel algorithm
	    xrange_min = (xrange_min < 0) ? 0: xrange_min;
	    xrange_max = (xrange_max == DEFAULT_RIGHT_BORDER) || (xrange_max > width1) ? width1 : xrange_max;
	    maxX1 = minX1 + xrange_max;
	    minX1 += xrange_min;
	    width1 = maxX1 - minX1;
	    int minX2 = std::max(minX1 - maxD, 0), maxX2 = std::min(maxX1 - minD, width);
	    int width2 = maxX2 - minX2;
	    const PixType *row1 = img1.ptr<PixType>(y), *row2 = img2.ptr<PixType>(y);
	    PixType *prow1 = buffer + width2*2, *prow2 = prow1 + width*cn*2;
	    tab += tabOfs;

	    for( c = 0; c < cn*2; c++ )
	      {
	    prow1[width*c] = prow1[width*c + width-1] =
	      prow2[width*c] = prow2[width*c + width-1] = tab[0];
	  }


	    int n1 = y > 0 ? -(int)img1.stride : 0, s1 = y < img1.rows-1 ? (int)img1.stride : 0;
	    int n2 = y > 0 ? -(int)img2.stride : 0, s2 = y < img2.rows-1 ? (int)img2.stride : 0;
	    int minX_cmn = std::min(minX1,minX2)-1;
	    int maxX_cmn = std::max(maxX1,maxX2)+1;
	    minX_cmn = std::max(minX_cmn, 1);
	    maxX_cmn = std::min(maxX_cmn, width - 1);
	    const v16i8 v_revert = {
	    15,14,13,12,
	      11,10, 9, 8,
	      7 , 6, 5, 4,
	      3 , 2, 1, 0
	      };
	    if( cn == 1 )
	      {
	    int end = (maxX_cmn - minX_cmn) / 16 * 16;
	    x = minX_cmn;
	    for(x = minX_cmn;x < end;x += 16)
	      {
	    v16u8 v_row1[3];
	    v16u8 v_row1_n[3];
	    v16u8 v_row1_s[3];


	    LD_ROW(x,row1,v_row1);
	    LD_ROW(x + n1,row1,v_row1_n);
	    LD_ROW(x + s1,row1,v_row1_s);


	    v8i16 v_index1[2];

	    CAL_INDEX(v_row1,v_row1_n,v_row1_s);

	    ST_PROW(prow1,x,0,0);  ST_PROW(prow1,x,1,1);  ST_PROW(prow1,x,2,2);  ST_PROW(prow1,x,3,3);
	    ST_PROW(prow1,x,4,4);  ST_PROW(prow1,x,5,5);  ST_PROW(prow1,x,6,6);  ST_PROW(prow1,x,7,7);
	    ST_PROW(prow1,x,8,8);  ST_PROW(prow1,x,9,9);  ST_PROW(prow1,x,10,10);ST_PROW(prow1,x,11,11);
	    ST_PROW(prow1,x,12,12);ST_PROW(prow1,x,13,13);ST_PROW(prow1,x,14,14);ST_PROW(prow1,x,15,15);
	    __msa_st_b(v_row1[1],&prow1[x+width],0);


	    v16u8 v_row2[3];
	    v16u8 v_row2_n[3];
	    v16u8 v_row2_s[3];

	    LD_ROW(x,row2,v_row2);
	    LD_ROW(x + n2,row2,v_row2_n);
	    LD_ROW(x + s2,row2,v_row2_s);

	    CAL_INDEX(v_row2,v_row2_n,v_row2_s);

	    int dx = width-1-x - 15;
	    ST_PROW(prow2,dx,0,15);  ST_PROW(prow2,dx,1,14);  ST_PROW(prow2,dx,2,13);  ST_PROW(prow2,dx,3,12);
	    ST_PROW(prow2,dx,4,11);  ST_PROW(prow2,dx,5,10);  ST_PROW(prow2,dx,6,9);   ST_PROW(prow2,dx,7,8);
	    ST_PROW(prow2,dx,8,7);   ST_PROW(prow2,dx,9,6);   ST_PROW(prow2,dx,10,5);  ST_PROW(prow2,dx,11,4);
	    ST_PROW(prow2,dx,12,3);  ST_PROW(prow2,dx,13,2);  ST_PROW(prow2,dx,14,1);  ST_PROW(prow2,dx,15,0);

	    v16u8 v_d;

	    //v_d = __msa_vshfr_b(v_row2[1],v_row2[1],v_revert);
	    v_d = __msa_vshf_b(v_revert,v_row2[1],v_row2[1]);
	    __msa_st_b(v_d,&prow2[width-1-x - 15 + width],0);
	  }

	    for(; x < maxX_cmn; x++ )
	      {
	    prow1[x] = tab[(row1[x+1] - row1[x-1])*2 + row1[x+n1+1] - row1[x+n1-1] + row1[x+s1+1] - row1[x+s1-1]];
	    prow2[width-1-x] = tab[(row2[x+1] - row2[x-1])*2 + row2[x+n2+1] - row2[x+n2-1] + row2[x+s2+1] - row2[x+s2-1]];

	    prow1[x+width] = row1[x];
	    prow2[width-1-x+width] = row2[x];
	  }

	    // printf("--->\n");
	    // for( x = minX_cmn; x < maxX_cmn; x++ )
	    // {
	    // 	printf("%3d ",prow2[width-1-x+width]);
	    // }
	    // printf("\n");
	  }
	    //printf("== %d\n",width1*D*sizeof(cost[0]));
	    //memset( cost + xrange_min*D, 0, width1*D*sizeof(cost[0]) );
	    buffer -= width-1-maxX2;
	    cost -= (minX1-xrange_min)*D + minD; // simplify the cost indices inside the loop
	    cal_prow2buffer(minX2,maxX2,prow2,buffer,width,width2);
	    cal_cost<0>(prow1,prow2,buffer,minX1,maxX1,minD,maxD,width,width2,cost,D);
	    prow1 += width, prow2 += width;
	    cal_prow2buffer(minX2,maxX2,prow2,buffer,width,width2);
	    cal_cost<2>(prow1,prow2,buffer,minX1,maxX1,minD,maxD,width,width2,cost,D);

	  }
#endif
