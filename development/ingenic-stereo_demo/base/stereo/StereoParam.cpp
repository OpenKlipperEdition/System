#include <StereoParam.hpp>
#include <iostream>
using namespace std;
void StereoParam::dump(void)
{
  cout << "====== dumping Stereo Parameter ===== " << endl;
  cout << "left.fx =  " << left.fx << "," << endl
       << "left.fy = " << left.fy << "," << endl
       << "left.cx = " << left.cx << "," << endl
       << "left.cy = " << left.cy << "," << endl
       << "left.width = " << left.width << "," << endl
       << "left.height = " << left.height << "," << endl
       << "left.skew = " << left.skew << "," << endl
       << "left.t1 = " << left.t1 << "," << endl
       << "left.t2 = " << left.t2 << "," << endl
       << "left.radial[0] = " << left.radial[0] << "," << endl
       << "left.radial[1] = " << left.radial[1] << "," << endl;

  cout << "right.fx =  " << right.fx << "," << endl
       << "right.fy = " << right.fy << "," << endl
       << "right.cx = " << right.cx << "," << endl
       << "right.cy = " << right.cy << "," << endl
       << "right.width = " << right.width << "," << endl
       << "right.height = " << right.height << "," << endl
       << "right.skew = " << right.skew << "," << endl
       << "right.t1 = " << right.t1 << "," << endl
       << "right.t2 = " << right.t2 << "," << endl
       << "right.radial[0] = " << right.radial[0] << "," << endl
       << "right.radial[1] = " << right.radial[1] << "," << endl;

  cout << "rightToLeft: rotate" << endl;
  for (int i = 0; i < 9; i++)
    cout << rightToLeft.R[i] << endl;
  cout << "rightToLeft: x = " << rightToLeft.T[0] << endl;
  cout << "rightToLeft: y = " << rightToLeft.T[1] << endl;
  cout << "rightToLeft: z = " << rightToLeft.T[2] << endl;
}
