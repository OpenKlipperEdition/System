#include <ImgWarp.hpp>
#include <cvdefs.hpp>
#include <saturate_cast.hpp>
#include <iostream>
#include <cassert>
#include <IMat.hpp>
using namespace internal;
using namespace std;
using namespace JzStereo;

namespace JzStereo_internal {

template<typename ST, typename DT> struct Cast
{
    typedef ST type1;
    typedef DT rtype;

  DT operator()(ST val) const { return saturate_cast<DT>(val); }
};

template<typename ST, typename DT, int bits> struct FixedPtCast
{
    typedef ST type1;
    typedef DT rtype;
    enum { SHIFT = bits, DELTA = 1 << (bits-1) };

    DT operator()(ST val) const { return saturate_cast<DT>((val + DELTA)>>SHIFT); }
};

struct RemapNoVec
{
  int operator()( const JzStereo::IMat&, void*, const short*, const ushort*,
                    const void*, int ) const { return 0; }
};

#if SIMD128
struct RemapVec_8u
{
  int operator()( // const Mat& _src, void* _dst, const short* XY,
		  // const ushort* FXY, const void* _wtab, int width
		  ) const
  {
  }
};
#else
typedef RemapNoVec RemapVec_8u;
#endif

const int INTER_REMAP_COEF_BITS=15;
const int INTER_REMAP_COEF_SCALE=1 << INTER_REMAP_COEF_BITS;

//static uchar NNDeltaTab_i[INTER_TAB_SIZE2][2];

static float BilinearTab_f[INTER_TAB_SIZE2][2][2];
static short BilinearTab_i[INTER_TAB_SIZE2][2][2];


static inline void interpolateLinear( float x, float* coeffs )
{
    coeffs[0] = 1.f - x;
    coeffs[1] = x;
}

static void initInterTab1D(int method, float* tab, int tabsz)
{
    float scale = 1.f/tabsz;
    if( method == INTER_LINEAR )
    {
        for( int i = 0; i < tabsz; i++, tab += 2 )
            interpolateLinear( i*scale, tab );
    }
    else
      cout << "Unknown interpolation method"  << endl;
}

/*
 Various border types, image boundaries are denoted with '|'

 * BORDER_REPLICATE:     aaaaaa|abcdefgh|hhhhhhh
 * BORDER_REFLECT:       fedcba|abcdefgh|hgfedcb
 * BORDER_REFLECT_101:   gfedcb|abcdefgh|gfedcba
 * BORDER_WRAP:          cdefgh|abcdefgh|abcdefg
 * BORDER_CONSTANT:      iiiiii|abcdefgh|iiiiiii  with some specified 'i'
 */
int borderInterpolate( int p, int len, int borderType )
{
    if( (unsigned)p < (unsigned)len )
        ;
    else if( borderType == BORDER_REPLICATE )
        p = p < 0 ? 0 : len - 1;
    else if( borderType == BORDER_REFLECT || borderType == BORDER_REFLECT_101 )
    {
        int delta = borderType == BORDER_REFLECT_101;
        if( len == 1 )
            return 0;
        do
        {
            if( p < 0 )
                p = -p - 1 + delta;
            else
                p = len - 1 - (p - len) - delta;
        }
        while( (unsigned)p >= (unsigned)len );
    }
    else if( borderType == BORDER_WRAP )
    {
        assert(len > 0);
        if( p < 0 )
            p -= ((p-len+1)/len)*len;
        if( p >= len )
            p %= len;
    }
    else if( borderType == BORDER_CONSTANT )
        p = -1;
    else
      fprintf( stderr, "Unknown/unsupported border type" );
    return p;
}

static const void* initInterTab2D( int method, bool fixpt )
{
    static bool inittab[INTER_MAX+1] = {false};
    float* tab = 0;
    short* itab = 0;
    int ksize = 0;
    if( method == INTER_LINEAR )
        tab = BilinearTab_f[0][0], itab = BilinearTab_i[0][0], ksize=2;
    else
      cout << "Unknown/unsupported interpolation type" << endl;

    if( !inittab[method] )
    {
      //        AutoBuffer<float> _tab(8*INTER_TAB_SIZE);
        float _tab[8*INTER_TAB_SIZE];
        int i, j, k1, k2;

        initInterTab1D(method, _tab, INTER_TAB_SIZE);

        for( i = 0; i < INTER_TAB_SIZE; i++ )
            for( j = 0; j < INTER_TAB_SIZE; j++, tab += ksize*ksize, itab += ksize*ksize )
            {
                int isum = 0;
                // NNDeltaTab_i[i*INTER_TAB_SIZE+j][0] = j < INTER_TAB_SIZE/2;
                // NNDeltaTab_i[i*INTER_TAB_SIZE+j][1] = i < INTER_TAB_SIZE/2;

                for( k1 = 0; k1 < ksize; k1++ )
                {
                    float vy = _tab[i*ksize + k1];
                    for( k2 = 0; k2 < ksize; k2++ )
                    {
                        float v = vy*_tab[j*ksize + k2];
                        tab[k1*ksize + k2] = v;
                        isum += itab[k1*ksize + k2] = saturate_cast<short>(v*INTER_REMAP_COEF_SCALE);
                    }
                }

                if( isum != INTER_REMAP_COEF_SCALE )
                {
                    int diff = isum - INTER_REMAP_COEF_SCALE;
                    int ksize2 = ksize/2, Mk1=ksize2, Mk2=ksize2, mk1=ksize2, mk2=ksize2;
                    for( k1 = ksize2; k1 < ksize2+2; k1++ )
                        for( k2 = ksize2; k2 < ksize2+2; k2++ )
                        {
                            if( itab[k1*ksize+k2] < itab[mk1*ksize+mk2] )
                                mk1 = k1, mk2 = k2;
                            else if( itab[k1*ksize+k2] > itab[Mk1*ksize+Mk2] )
                                Mk1 = k1, Mk2 = k2;
                        }
                    if( diff < 0 )
                        itab[Mk1*ksize + Mk2] = (short)(itab[Mk1*ksize + Mk2] - diff);
                    else
                        itab[mk1*ksize + mk2] = (short)(itab[mk1*ksize + mk2] - diff);
                }
            }
        tab -= INTER_TAB_SIZE2*ksize*ksize;
        itab -= INTER_TAB_SIZE2*ksize*ksize;
        inittab[method] = true;
    }
    return fixpt ? (const void*)itab : (const void*)tab;
}


static bool initAllInterTab2D()
{
    return  initInterTab2D( INTER_LINEAR, false ) &&
            initInterTab2D( INTER_LINEAR, true ) // &&
            // initInterTab2D( INTER_CUBIC, false ) &&
            // initInterTab2D( INTER_CUBIC, true ) &&
            // initInterTab2D( INTER_LANCZOS4, false ) &&
            // initInterTab2D( INTER_LANCZOS4, true )
    ;
}
static volatile bool doInitAllInterTab2D = initAllInterTab2D();

typedef void (*RemapFunc)(const IMat& _src, IMat& _dst, const IMat& _xy,
                          const IMat& _fxy, const void* _wtab,
                          int borderType/*, const Scalar& _borderValue*/);

template<class CastOp, class VecOp, typename AT>
static void remapBilinear( const IMat& _src, IMat& _dst, const IMat& _xy,
                           const IMat& _fxy, const void* _wtab,
                           int borderType //,  const Scalar& _borderValue
			   )
{
    typedef typename CastOp::rtype T;
    typedef typename CastOp::type1 WT;
    //    Size ssize = _src.size(), dsize = _dst.size();
    const int cn = _src.channels();
    const AT* wtab = (const AT*)_wtab;
    const T* S0 = _src.ptr<T>();
    size_t sstep = _src.stride;

    T cval[IMat_CN_MAX];
    CastOp castOp;
    VecOp vecOp;

    for(int k = 0; k < cn; k++ )
        // cval[k] = saturate_cast<T>(_borderValue[k & 3]);
      cval[k] = 0;
    unsigned width1 = std::max(_src.width-1, 0), height1 = std::max(_src.height-1, 0);
    assert( !_src.empty() );

#if SIMD128
    if( _src.type() == IMat_8UC3 )
        width1 = std::max(_src.width-2, 0);
#endif

    for(int dy = 0; dy < _dst.height; dy++ )
    {
        T* D = _dst.ptr<T>(dy);
        const short* XY = _xy.ptr<short>(dy);
        const ushort* FXY = _fxy.ptr<ushort>(dy);
        int X0 = 0;
        bool prevInlier = false;

        for(int dx = 0; dx <= _dst.width; dx++ )
        {
            bool curInlier = dx < _dst.width ?
                (unsigned)XY[dx*2] < width1 &&
                (unsigned)XY[dx*2+1] < height1 : !prevInlier;
            if( curInlier == prevInlier )
                continue;

            int X1 = dx;
            dx = X0;
            X0 = X1;
            prevInlier = curInlier;

            if( !curInlier )
            {
	      
                int len = vecOp( _src, D, XY + dx*2, FXY + dx, wtab, X1 - dx );
                D += len*cn;
                dx += len;
		
                if( cn == 1 )
                {
                    for( ; dx < X1; dx++, D++ )
                    {
                        int sx = XY[dx*2], sy = XY[dx*2+1];

                        const AT* w = wtab + FXY[dx]*4;

                        const T* S = S0 + sy*sstep + sx;
                        *D = castOp(WT(S[0]*w[0] + S[1]*w[1] + S[sstep]*w[2] + S[sstep+1]*w[3]));
                    }
                }
                else if( cn == 2 )
                    for( ; dx < X1; dx++, D += 2 )
                    {
                        int sx = XY[dx*2], sy = XY[dx*2+1];
                        const AT* w = wtab + FXY[dx]*4;
                        const T* S = S0 + sy*sstep + sx*2;
                        WT t0 = S[0]*w[0] + S[2]*w[1] + S[sstep]*w[2] + S[sstep+2]*w[3];
                        WT t1 = S[1]*w[0] + S[3]*w[1] + S[sstep+1]*w[2] + S[sstep+3]*w[3];
                        D[0] = castOp(t0); D[1] = castOp(t1);
                    }
                else if( cn == 3 )
                    for( ; dx < X1; dx++, D += 3 )
                    {
                        int sx = XY[dx*2], sy = XY[dx*2+1];
                        const AT* w = wtab + FXY[dx]*4;
                        const T* S = S0 + sy*sstep + sx*3;
                        WT t0 = S[0]*w[0] + S[3]*w[1] + S[sstep]*w[2] + S[sstep+3]*w[3];
                        WT t1 = S[1]*w[0] + S[4]*w[1] + S[sstep+1]*w[2] + S[sstep+4]*w[3];
                        WT t2 = S[2]*w[0] + S[5]*w[1] + S[sstep+2]*w[2] + S[sstep+5]*w[3];
                        D[0] = castOp(t0); D[1] = castOp(t1); D[2] = castOp(t2);
                    }
                else if( cn == 4 )
                    for( ; dx < X1; dx++, D += 4 )
                    {
                        int sx = XY[dx*2], sy = XY[dx*2+1];
                        const AT* w = wtab + FXY[dx]*4;
                        const T* S = S0 + sy*sstep + sx*4;
                        WT t0 = S[0]*w[0] + S[4]*w[1] + S[sstep]*w[2] + S[sstep+4]*w[3];
                        WT t1 = S[1]*w[0] + S[5]*w[1] + S[sstep+1]*w[2] + S[sstep+5]*w[3];
                        D[0] = castOp(t0); D[1] = castOp(t1);
                        t0 = S[2]*w[0] + S[6]*w[1] + S[sstep+2]*w[2] + S[sstep+6]*w[3];
                        t1 = S[3]*w[0] + S[7]*w[1] + S[sstep+3]*w[2] + S[sstep+7]*w[3];
                        D[2] = castOp(t0); D[3] = castOp(t1);
                    }
                else
                    for( ; dx < X1; dx++, D += cn )
                    {
                        int sx = XY[dx*2], sy = XY[dx*2+1];
                        const AT* w = wtab + FXY[dx]*4;
                        const T* S = S0 + sy*sstep + sx*cn;
                        for(int k = 0; k < cn; k++ )
                        {
                            WT t0 = S[k]*w[0] + S[k+cn]*w[1] + S[sstep+k]*w[2] + S[sstep+k+cn]*w[3];
                            D[k] = castOp(t0);
                        }
                    }
            }
            else
            {
                if( cn == 1 )
                    for( ; dx < X1; dx++, D++ )
                    {
                        int sx = XY[dx*2], sy = XY[dx*2+1];
                        if( borderType == BORDER_CONSTANT &&
                            (sx >= _src.width || sx+1 < 0 ||
                             sy >= _src.height || sy+1 < 0) )
                        {
                            D[0] = cval[0];
                        }
                        else
                        {
                            int sx0, sx1, sy0, sy1;
                            T v0, v1, v2, v3;
                            const AT* w = wtab + FXY[dx]*4;
                            {
                                sx0 = borderInterpolate(sx, _src.width, borderType);
                                sx1 = borderInterpolate(sx+1, _src.width, borderType);
                                sy0 = borderInterpolate(sy, _src.height, borderType);
                                sy1 = borderInterpolate(sy+1, _src.height, borderType);
                                v0 = sx0 >= 0 && sy0 >= 0 ? S0[sy0*sstep + sx0] : cval[0];
                                v1 = sx1 >= 0 && sy0 >= 0 ? S0[sy0*sstep + sx1] : cval[0];
                                v2 = sx0 >= 0 && sy1 >= 0 ? S0[sy1*sstep + sx0] : cval[0];
                                v3 = sx1 >= 0 && sy1 >= 0 ? S0[sy1*sstep + sx1] : cval[0];
                            }
                            D[0] = castOp(WT(v0*w[0] + v1*w[1] + v2*w[2] + v3*w[3]));
                        }
                    }
                else
                    for( ; dx < X1; dx++, D += cn )
                    {
                        int sx = XY[dx*2], sy = XY[dx*2+1];
                        if( borderType == BORDER_CONSTANT &&
                            (sx >= _src.width || sx+1 < 0 ||
                             sy >= _src.height || sy+1 < 0) )
                        {
                            for(int k = 0; k < cn; k++ )
                                D[k] = cval[k];
                        }
                        else
                        {
                            int sx0, sx1, sy0, sy1;
                            const T *v0, *v1, *v2, *v3;
                            const AT* w = wtab + FXY[dx]*4;
                            {
                                sx0 = borderInterpolate(sx, _src.width, borderType);
                                sx1 = borderInterpolate(sx+1, _src.width, borderType);
                                sy0 = borderInterpolate(sy, _src.height, borderType);
                                sy1 = borderInterpolate(sy+1, _src.height, borderType);
                                v0 = sx0 >= 0 && sy0 >= 0 ? S0 + sy0*sstep + sx0*cn : &cval[0];
                                v1 = sx1 >= 0 && sy0 >= 0 ? S0 + sy0*sstep + sx1*cn : &cval[0];
                                v2 = sx0 >= 0 && sy1 >= 0 ? S0 + sy1*sstep + sx0*cn : &cval[0];
                                v3 = sx1 >= 0 && sy1 >= 0 ? S0 + sy1*sstep + sx1*cn : &cval[0];
                            }
                            for(int k = 0; k < cn; k++ )
                                D[k] = castOp(WT(v0[k]*w[0] + v1[k]*w[1] + v2[k]*w[2] + v3[k]*w[3]));
                        }
                    }
            }
        }
    }
}

void remap( IMat &_src, IMat &_dst, IMat &_map1, IMat &_map2,
	    int interpolation, int borderType /*, const Scalar& borderValue */)
{
  const void *ctab = 0;
  RemapFunc ifunc;
  static RemapFunc linear_tab[] =
    {
      remapBilinear<FixedPtCast<int, uchar, INTER_REMAP_COEF_BITS>, RemapVec_8u, short>
    };

  _dst = IMat(_src.width, _src.height, _src.type);
  if (interpolation == INTER_LINEAR)
    ifunc = linear_tab[0];

  // for 8U no matter how many channels.
  // TODO: re-write this IMat type check.
  bool fixpt = (_src.type == IMat_8U
		|| _src.type == IMat_8UC2 || _src.type == IMat_8UC3);

  ctab = initInterTab2D( interpolation, fixpt );
  ifunc( _src, _dst, _map1, _map2, ctab, borderType /*, const Scalar& _borderValue*/);
}
} // JzStereo_internal
