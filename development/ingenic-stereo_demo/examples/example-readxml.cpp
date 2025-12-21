#include <yaml-cpp/yaml.h>
#include <iostream>


using namespace std;
using namespace YAML;

#if USE_OPENCV
#ifdef OPENCV_GUI
#include "opencv2/highgui.hpp"
#endif
#include "opencv2/imgcodecs.hpp"
#include "opencv2/imgproc.hpp"


void read_yaml_cpp()
{
  string mapfileL("/data1/home/qianliu/work/opencv/examples/mysample/re-write/build2/left.yaml");
  string mapfileR("/data1/home/qianliu/work/opencv/examples/mysample/re-write/build2/right.yaml");

  YAML::Node top;
  top = YAML::LoadFile(mapfileL);
  cout << top["rmapl0"]["rows"] << endl;
}

void read_yaml_opencv()
{
  cv::Mat mapL0, mapL1, mapR0, mapR1;

  string mapfileL("/data1/home/qianliu/work/opencv/examples/mysample/re-write/build2/left.yaml");
  string mapfileR("/data1/home/qianliu/work/opencv/examples/mysample/re-write/build2/right.yaml");
  cv::FileStorage lmap(mapfileL, cv::FileStorage::READ);
  cv::FileStorage rmap(mapfileR, cv::FileStorage::READ);

  int rows;
  lmap["rows"] >> rows;
  cout << rows << endl;

  // lmap["rmapl0"] >> mapL0;
  // lmap["rmapl1"] >> mapL1;
  // rmap["rmapr0"] >> mapR0;
  // rmap["rmapr1"] >> mapR1;
}

#endif

#include <persistence.hpp>
using namespace JzStereo;
void read_yaml_local()
{
  IMat mapL0;
  string mapfileL("/data1/home/qianliu/work/opencv/examples/mysample/re-write/build2/left.yaml");
  FileStorage lmap(mapfileL, FileStorage::READ);  
}

int main(int argc, char **argv)
{
  read_yaml_local();
#if USE_OPENCV
  read_yaml_opencv();
#endif
}
