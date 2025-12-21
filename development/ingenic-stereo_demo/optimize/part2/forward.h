#ifndef FORWORD_H
#define FORWORD_H

#include "gendata_template.h"
#include <IMat.hpp>
#include <StereoDisparity.hpp>
using namespace JzStereo;

class DataInit :public TestData
{
public:
	//InputData.
        SgbmDisparityParam *params;
	IMat img1, img2, disp1;
	unsigned char * data1, *data2;
	signed short*data3;
public:
	//    DataInit(int rand = 0):TestData(rand){
         DataInit(int rand = 0):TestData(rand){
	}
private:

	virtual void InitData()
	{
	  {
	  SgbmDisparityParam *sgbmParam = new SgbmDisparityParam();
	  sgbmParam->minDisparity = 0;
	  sgbmParam->numDisparities = 32;
	  sgbmParam->numDisparities = 128;
		  
	  /* min_disparity = 0; */
	  /* range_disparity = sgbmParam->numDisparities; */
	  sgbmParam->SADWindowSize = 5;
	  sgbmParam->preFilterCap = 63;
	  sgbmParam->uniquenessRatio = 10;
	  sgbmParam->P1 = 600;
	  sgbmParam->P2 = 2400;
	  sgbmParam->speckleWindowSize = 100;
	  sgbmParam->speckleRange = 32;
	  sgbmParam->disp12MaxDiff = 1;
	  sgbmParam->mode = 0;
	  params = sgbmParam;
	  }

	  /* int col = 64; */
	  /* int row = 4; */

	  int col = 640;
	  int row = 480;

	  int type = IMat_8U;

	  genUInt8.SetDataMin(0);
	  genUInt8.SetDataRange(255);	  
	  data1 = genUInt8.randMalloc(col * row,  "img1");
	  img1 = IMat(data1, col, row, type);

	  data2 = genUInt8.randMalloc(col * row,  "img2");
	  img2 = IMat(data2, col, row, type);

	  genInt16.SetDataMin(0x8000);
	  genInt16.SetDataRange(0x7ff);
	  data3 = genInt16.randMalloc(col * row,  "disp");
	  disp1 = IMat(data3, col, row, IMat_16S);

	}
};
typedef void (*FUNC)(IMat &img1, IMat &img2, IMat &disp1, SgbmDisparityParam *params);
class A : public DataInit
{
public:
    A(int rand = 0):DataInit(rand){

	}
	static FUNC test;
	virtual void Test()
	{
	  test(img1, img2, disp1, params);
	}
};

class B : public DataInit
{
public:

    B(int rand = 0):DataInit(rand){

	}
	static FUNC test;
	virtual void Test()
	{

		 test(img1, img2, disp1, params);
	}
};

class C : public DataInit
{
public:

    C(int rand = 0):DataInit(rand) {

  }
	static FUNC test;
	virtual void Test()
	{

	  test(img1, img2, disp1, params);
	}
};


void RegisterA(FUNC a);
void RegisterB(FUNC b);
void RegisterC(FUNC c);

class Register{
public:
	Register(int t,FUNC a){
		if(t == 0){
		  printf("regiter A\n");
			RegisterA(a);
		}else if (t == 1){
		  printf("regiter B\n");
			RegisterB(a);
		} else if (t == 2)
		  {
		    printf("regiter C\n");
		    RegisterC(a);
		  }
	}
};
#endif /* FORWORD_H */
