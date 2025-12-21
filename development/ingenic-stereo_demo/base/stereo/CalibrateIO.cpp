#include <CalibrateIO.hpp>
#include <yaml-cpp/yaml.h>
#include <iostream>
using namespace std;
using namespace  YAML;
string CalibrateIO::MODEL_STEREO = "stereo_camera";
string CalibrateIO::MODEL_PINHOLE = "pinhole_radial_tangential";



StereoParam &CalibrateIO::load(std::string paramFile)
{
  YAML::Node top;
  top = LoadFile(paramFile);
  
  StereoParam &param = load<StereoParam>(top);
  return param;
}

template<typename T>
T &CalibrateIO::load(YAML::Node top)
{
  string mode;
  try
    {
      mode = top["model"].as<string>();

      if (mode == MODEL_STEREO)
	{
	  StereoParam *param = new StereoParam();
	  Node rightToLeft = top["rightToLeft"];
      
	  param->left = load<CameraPinholeBrow>(top["left"]);
	  param->right = load<CameraPinholeBrow>(top["right"]);
	  param->rightToLeft = top["rightToLeft"].as<Transform>();
	  //	  param->dump();
	  Node QNode = top["Q"];
	  double *pq = param->Q;
	  if (QNode.IsSequence())
	    for (int i = 0; i < QNode.size(); ++i)
	      pq[i] = QNode[i].as<double>();
	  else
	    for (int i = 0; i < sizeof(param->Q)/sizeof(double); ++i)
	      pq[i] = 0.0f;

	  return  (T &)*param;
	}
      else if (mode == MODEL_PINHOLE)
	{
	  CameraPinholeBrow *param = new CameraPinholeBrow();
	  *param = top.as<CameraPinholeBrow>();      
	  return (T &)*param;
	}
      else
	{
	  T *param = new T();
	  printf("unsupport mode %s\n", mode.c_str());
	  return (T &)*param;
	}
    }
  catch (...)
    {
      T *param = new T();
      printf("yaml parse fail\n");
      return (T &)*param;
    }     
}


/*
  StereoParam & CalibrateIO::load(std::string paramFile)
  {
  YAML::Node top = YAML::LoadFile(paramFile);
  StereoParam *param = new StereoParam();
  
  string mode = top["model"].as<string>();
  if ( == MODEL_STEREO)
  {
  printf("support mode %s\n", top["model"].as<string>());
  }
  else
  {
  printf("unsupport mode %s\n", top["model"].as<string>());
  }
  return *param;
  }

*/
