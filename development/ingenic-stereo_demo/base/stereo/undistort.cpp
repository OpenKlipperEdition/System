#include <iostream>
#include <math.h>

#include "IMat.hpp"
#include "ImgWarp.hpp"
#include "saturate_cast.hpp"

using namespace internal;
using namespace std;
using namespace JzStereo_internal;


namespace JzStereo{

	IMat IMatMul(IMat A,IMat B){
		IMat C(A.height ,B.width,IMat_64F);
		if (A.height != B.width){
			cout << " Error : width != height!" << endl;
			exit(1);
		}else{
			double *adata=A.ptr<double>();
			double *bdata=B.ptr<double>();
			double *cdata=C.ptr<double>();
			gemm<double>(A.height,B.width,A.width,adata,bdata,cdata);
		}
		return C;
	}


	void computeTiltProjectionMatrix(double tauX, double tauY, IMat* matTilt){
		double cTauX = cos(tauX);
		double sTauX = sin(tauX);
		double cTauY = cos(tauY);
		double sTauY = sin(tauY);
		double matRotX_data[3][3] = {1,0,0,0,cTauX,sTauX,0,-sTauX,cTauX};
		double matRotY_data[3][3] = {cTauY,0,-sTauY,0,1,0,sTauY,0,cTauY};
		IMat matRotX(matRotX_data,3,3,IMat_64F);
		IMat matRotY(matRotY_data,3,3,IMat_64F);
		IMat matRotXY = IMatMul(matRotY , matRotX);
		double matProjZ_data[3][3] = {matRotXY.at<double>(2,2),0,-matRotXY.at<double>(0,2),0,matRotXY.at<double>(2,2),-matRotXY.at<double>(1,2),0,0,1};
		IMat matProjZ(matProjZ_data,3,3,IMat_64F);
		if (matTilt)
		{
			// Matrix for trapezoidal distortion of tilted image sensor
			*matTilt = IMatMul(matProjZ , matRotXY);
		}

	}


#define det3(m)   (m(0,0)*((double)m(1,1)*m(2,2) - (double)m(1,2)*m(2,1)) -  \
		m(0,1)*((double)m(1,0)*m(2,2) - (double)m(1,2)*m(2,0)) +  \
		m(0,2)*((double)m(1,0)*m(2,1) - (double)m(1,1)*m(2,0)))
#define Sd( y, x ) ((double*)(srcdata + y*srcstep))[x]
#define Dd( y, x ) ((double*)(dstdata + y*dststep))[x]

	IMat IMatInvert(IMat src,IMat dst){
		const uchar* srcdata = src.ptr<uchar>();
		uchar* dstdata = dst.ptr<uchar>();
		size_t srcstep = 24;
		size_t dststep = 24;

		double d = det3(Sd);
		if( d != 0. ){
			//	result = true;
			d = 1./d;
			double t[9];

			t[0] = (Sd(1,1) * Sd(2,2) - Sd(1,2) * Sd(2,1)) * d;
			t[1] = (Sd(0,2) * Sd(2,1) - Sd(0,1) * Sd(2,2)) * d;
			t[2] = (Sd(0,1) * Sd(1,2) - Sd(0,2) * Sd(1,1)) * d;

			t[3] = (Sd(1,2) * Sd(2,0) - Sd(1,0) * Sd(2,2)) * d;
			t[4] = (Sd(0,0) * Sd(2,2) - Sd(0,2) * Sd(2,0)) * d;
			t[5] = (Sd(0,2) * Sd(1,0) - Sd(0,0) * Sd(1,2)) * d;

			t[6] = (Sd(1,0) * Sd(2,1) - Sd(1,1) * Sd(2,0)) * d;
			t[7] = (Sd(0,1) * Sd(2,0) - Sd(0,0) * Sd(2,1)) * d;
			t[8] = (Sd(0,0) * Sd(1,1) - Sd(0,1) * Sd(1,0)) * d;

			Dd(0,0) = t[0]; Dd(0,1) = t[1]; Dd(0,2) = t[2];
			Dd(1,0) = t[3]; Dd(1,1) = t[4]; Dd(1,2) = t[5];
			Dd(2,0) = t[6]; Dd(2,1) = t[7]; Dd(2,2) = t[8];
		}
		return dst;
	}

	IMat IMatColRange(IMat A, int m,int n){
		IMat B(n,A.height,IMat_64F);
		for(int i=0; i<A.height; i++){
			for(int j=m; j<n; j++){
				B.at<double>(i,j) = A.at<double>(i,j);
			}
		}
		return B;
	}

	IMat getDefaultNewCameraMatrx(IMat &cameraMatrix, Size imgsize,bool centerPrincipalPoint ){
		if( !centerPrincipalPoint && cameraMatrix.type == IMat_64F )
			return cameraMatrix;

		IMat newCameraMatrix;
		cameraMatrix.convertTo(newCameraMatrix, IMat_64F,0,0);
		if( centerPrincipalPoint )
		{ //NEED-CHECK
			newCameraMatrix.at<double>(0,2) = (double)((imgsize.width-1)*0.5);
			newCameraMatrix.at<double>(1,2) = (double)((imgsize.height-1)*0.5);
		}
		return newCameraMatrix;
	}

	void initUndistortRectifyMap( IMat &cameraMatrix, IMat &distCoeffs,
			IMat &matR, IMat &newCameraMatrix,
			Size size, int m1type, IMat &map1, IMat &map2 ){

		//m1typ,m2type
		if( m1type <= 0 )
			m1type = IMat_16SC2;
		JZ_Assert( m1type == IMat_16SC2 || m1type == IMat_32F || m1type == IMat_32FC2 );
		map1 =IMat(size.width, size.height, m1type );
		if( m1type != IMat_32FC2 )
		{
			map2 = IMat(size.width, size.height, m1type == IMat_16SC2 ? IMat_16U :IMat_32F);
		}
		else
			map2.release();

		//R,Ar
		double eye_data[3][3]={1.0,0.,0.,0.,1.0,0.,0.,0.,1.0};
		IMat R(eye_data, 3, 3, IMat_64F);

		IMat A, Ar;
		cameraMatrix.convertTo(A,IMat_64F,1,0);

		if( !newCameraMatrix.empty() )
			newCameraMatrix.convertTo(Ar,IMat_64F,1,0);
		else
			Ar = getDefaultNewCameraMatrx(A,size,true);

		if( !matR.empty() )
			matR.convertTo(R,IMat_64F,1,0);

		if( !distCoeffs.empty() )
			distCoeffs.convertTo(distCoeffs,IMat_64F,1,0);
		else{
			double zero_data[14][1]={0.};
			distCoeffs = IMat(zero_data,14, 1, IMat_64F);
		}

		JZ_Assert( (A.width == 3 ) && ( A.height == 3 ) && (R.width == 3 )&& ( R.height == 3 ));
		JZ_Assert( ((Ar.width == 3) && (Ar.height == 3)) || ((Ar.width == 4) && (Ar.height == 3)) );

		IMat tmpR = IMatMul(IMatColRange(Ar,0,3), R);
		double dst_data[9]={0.};
		IMat dst = IMat(dst_data,3,3,IMat_64F);
		IMat iR = IMatInvert(tmpR,dst);
		const double* ir =iR.ptr<double>();

		double u0 = A.at<double>(0, 2),  v0 = A.at<double>(1, 2);
		double fx = A.at<double>(0, 0),  fy = A.at<double>(1, 1);

		JZ_Assert( (distCoeffs.width == 1 && distCoeffs.height == 4 ) ||
				(distCoeffs.width == 4 && distCoeffs.height == 1 ) ||
				(distCoeffs.width == 1 && distCoeffs.height == 5 ) ||
				(distCoeffs.width == 5 && distCoeffs.height == 1 ) ||
				(distCoeffs.width == 1 && distCoeffs.height == 8 ) ||
				(distCoeffs.width == 8 && distCoeffs.height == 1 ) ||
				(distCoeffs.width == 1 && distCoeffs.height == 12 ) ||
				(distCoeffs.width == 12 && distCoeffs.height == 1 ) ||
				(distCoeffs.width == 1 && distCoeffs.height == 14 ) ||
				(distCoeffs.width == 14 && distCoeffs.height == 1 ) );

		//FIXME
		//		if( distCoeffs.rows != 1 && !distCoeffs.isContinuous() )
		//			distCoeffs = distCoeffs.t();

		const double* const distPtr = distCoeffs.ptr<double>();
		double k1 = distPtr[0];
		double k2 = distPtr[1];
		double p1 = distPtr[2];
		double p2 = distPtr[3];
		double k3 = distCoeffs.cols + distCoeffs.rows - 1 >= 5 ? distPtr[4] : 0.;
		double k4 = distCoeffs.cols + distCoeffs.rows - 1 >= 8 ? distPtr[5] : 0.;
		double k5 = distCoeffs.cols + distCoeffs.rows - 1 >= 8 ? distPtr[6] : 0.;
		double k6 = distCoeffs.cols + distCoeffs.rows - 1 >= 8 ? distPtr[7] : 0.;
		double s1 = distCoeffs.cols + distCoeffs.rows - 1 >= 12 ? distPtr[8] : 0.;
		double s2 = distCoeffs.cols + distCoeffs.rows - 1 >= 12 ? distPtr[9] : 0.;
		double s3 = distCoeffs.cols + distCoeffs.rows - 1 >= 12 ? distPtr[10] : 0.;
		double s4 = distCoeffs.cols + distCoeffs.rows - 1 >= 12 ? distPtr[11] : 0.;
		double tauX = distCoeffs.cols + distCoeffs.rows - 1 >= 14 ? distPtr[12] : 0.;
		double tauY = distCoeffs.cols + distCoeffs.rows - 1 >= 14 ? distPtr[13] : 0.;

		// Matrix for trapezoidal distortion of tilted image sensor
		IMat matTilt (eye_data,3,3, IMat_64F);
		computeTiltProjectionMatrix(tauX, tauY, &matTilt);

		for( int i = 0; i < size.height; i++ ){
			float* m1f = map1.ptr<float>(i);
			float* m2f = map2.empty() ? 0 : map2.ptr<float>(i);
			short* m1 = (short*)m1f;
			ushort* m2 = (ushort*)m2f;
			double _x = i*ir[1] + ir[2], _y = i*ir[4] + ir[5], _w = i*ir[7] + ir[8];

			int j = 0;
			if (m1type == IMat_16SC2)
				JZ_Assert(m1 != NULL && m2 != NULL);
			else if (m1type == IMat_32F)
				JZ_Assert(m1f != NULL && m2f != NULL);
			else
				JZ_Assert(m1 != NULL);

			for( ; j < size.width; j++, _x += ir[0], _y += ir[3], _w += ir[6] ){
				double w = 1./_w, x = _x*w, y = _y*w;
				double x2 = x*x, y2 = y*y;
				double r2 = x2 + y2, _2xy = 2*x*y;
				double kr = (1 + ((k3*r2 + k2)*r2 + k1)*r2)/(1 + ((k6*r2 + k5)*r2 + k4)*r2);
				double xd = (x*kr + p1*_2xy + p2*(r2 + 2*x2) + s1*r2+s2*r2*r2);
				double yd = (y*kr + p1*(r2 + 2*y2) + p2*_2xy + s3*r2+s4*r2*r2);
				double Vec3d_data[3][1] = {xd,yd,1.0};

				IMat tmp_vec(Vec3d_data,3,1,IMat_64F);
				IMat vecTilt = IMatMul(matTilt , tmp_vec);

				double invProj = vecTilt.at<double>(0,2) ? 1./vecTilt.at<double>(0,2) : 1;
				double u = fx*invProj*vecTilt.at<double>(0,0) + u0;
				double v = fy*invProj*vecTilt.at<double>(0,1) + v0;

                vecTilt.release();

				if( m1type == IMat_16SC2 ){
					int iu = saturate_cast<int>(u*INTER_TAB_SIZE);
					int iv = saturate_cast<int>(v*INTER_TAB_SIZE);
					m1[j*2] = (short)(iu >> INTER_BITS);
					m1[j*2+1] = (short)(iv >> INTER_BITS);
					m2[j] = (ushort)((iv & (INTER_TAB_SIZE-1))*INTER_TAB_SIZE + (iu & (INTER_TAB_SIZE-1)));
				}
				else if( m1type == IMat_32F ){
					m1f[j] = (float)u;
					m2f[j] = (float)v;
				}
				else{
					m1f[j*2] = (float)u;
					m1f[j*2+1] = (float)v;
				}
			}
		}
	}
}//namespace
