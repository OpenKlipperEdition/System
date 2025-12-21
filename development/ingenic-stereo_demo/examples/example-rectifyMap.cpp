#include <iostream>
#include <fstream>
#include <string.h>
#include "IMat.hpp"
#include "undistort.hpp"
#include "UtilImageIO.hpp"

using namespace JzStereo;
using namespace std;


void save_IMat(IMat A ,string str,string name,int w,int h,int chal){
	ofstream outfile;
	int J = A.width*A.chal;
	outfile.open(str, ios::binary);
	outfile << name ;
	outfile << "[" << w << "]" << "[" << h << "]" << "[" << chal << "] = { ";
	for(int i=0; i<A.height; i ++){
		for(int j=0; j<J; j++){
			if(j % 20 == 0)
				outfile << "\n     ";
			outfile << A.at<short>(i,j);
			if(i==A.height-1 && j==J-1)
				outfile << " }; " << endl;
			else
				outfile << ", ";
		}
	}
	outfile.close();
}

int main()
{

	Size image_size;
	image_size.width = 640;
	image_size.height = 480;

	IMat mapx = IMat(image_size.width,image_size.height, IMat_16S);
	IMat mapy = IMat(image_size.width,image_size.height, IMat_16S);

	double M1_data[]={ 5.2567133048907908e+02, 0., 3.1577822205868762e+02, 0., 5.2845636741586054e+02, 2.4993429540701388e+02, 0., 0. ,1.};
	double D1_data[]={-8.2005378863977771e+00 ,-2.1551980629544037e+01, 1.0110792554016132e-02, -5.4923455176559641e-03,   2.1266194554952170e+02, -8.1045484781952375e+00, -2.2829924456293082e+01 ,2.1691863564839039e+02, 0., 0., 0., 0., 0., 0.};
	double R1_data[]={ 0.9995151842930378, 0.002963797615469154, -0.0309937456811798,-0.002935909284355769, 0.999995243434746, 0.0009452748581362283, 0.0309963998607765, -0.0008538217483352574, 0.9995191314747771};
	double P1_data[]={ 528.7892706041026, 0, 331.5748558044434, 0,0, 528.7892706041026, 257.0511722564697, 0, 0, 0, 1, 0};
	IMat M1(M1_data,3,3,IMat_64F);
	IMat D1(D1_data,1,14,IMat_64F);
	IMat R1(R1_data,3,3,IMat_64F);
	IMat P1(P1_data,4,3,IMat_64F);
	initUndistortRectifyMap(M1, D1, R1, P1, image_size, IMat_16SC2, mapx, mapy);
	save_IMat(mapx,"mapx.cpp","short matx",image_size.width,image_size.height,mapx.chal);
	save_IMat(mapy,"mapy.cpp","short maty",image_size.width,image_size.height,mapy.chal);
	return 0;

}

