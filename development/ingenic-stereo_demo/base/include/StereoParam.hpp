#ifndef __STEREOPARAM_H__
#define __STEREOPARAM_H__

#include "yaml-cpp/yaml.h"
#include <string>
class CameraPinholeBrow {
public:
  double *radial;
  double t1, t2;
  double fx, fy;
  double cx, cy;
  int width, height;
  double skew;
};

namespace YAML {
  template<>
  struct convert<CameraPinholeBrow> {
    static Node encode (const CameraPinholeBrow &rhs) {
      Node node;
      // TODO
      return node;
    }
    static bool decode (const Node &node, CameraPinholeBrow &rhs) {
      std::string mode = node["model"].as<std::string>();
      if ( mode != "pinhole_radial_tangential")
	{
	  return false;
	}
      Node pinholeNode = node["pinhole"];
      rhs.fx = pinholeNode["fx"].as<double>();
      rhs.fy = pinholeNode["fy"].as<double>();
      rhs.cx = pinholeNode["cx"].as<double>();
      rhs.cy = pinholeNode["cy"].as<double>();
      rhs.width = pinholeNode["width"].as<double>();
      rhs.height = pinholeNode["height"].as<double>();
      if (pinholeNode["skew"])
	rhs.skew = pinholeNode["skew"].as<double>();
      Node rtNode = node["radial_tangential"];
      if (rtNode.IsDefined())
	{
	  rhs.t1 =rtNode["t1"].as<double>();
	  rhs.t2 =rtNode["t2"].as<double>();
	  size_t s = rtNode["radial"].size();

	  if (s > 0)
	    {
	      rhs.radial = (double *)malloc(sizeof(double) * s);
	      for (int i = 0; i < s; i++)
		rhs.radial[i] = rtNode["radial"][i].as<double>();
	    }
	  else
	    {
	      rhs.radial = (double *)malloc(sizeof(double));
	      rhs.radial[0] = rtNode["radial"].as<double>();
	    }
	}
    else
      {
	return false;
      }
      return true;
    }
  };
}
class Transform {
public:
  double R[9];
  double T[3];
};


namespace YAML {
  template<>
  struct convert<Transform> {
    static Node encode (const Transform &rhs) {
      Node node;
      // TODO
      return node;
    }
    static bool decode (const Node &node, Transform &rhs) {
      Node rotationNode = node["rotation"];
      if (!rotationNode.IsSequence() || rotationNode.size() <= 0)
	  return false;
      for (int i = 0; i < rotationNode.size(); i++)
	  rhs.R[i] = rotationNode[i].as<double>();
      rhs.T[0] = node["x"].as<double>();
      rhs.T[1] = node["y"].as<double>();
      rhs.T[2] = node["z"].as<double>();
      return true;
    }    
  };
}
class StereoParam {
public:
  CameraPinholeBrow left;
  CameraPinholeBrow right;
  Transform rightToLeft;
  double Q[4*4];
  StereoParam (){};
  ~StereoParam (){};
  void dump(void);
  CameraPinholeBrow getLeft(){return left;};
  CameraPinholeBrow getRight(){return right;};
  Transform getRightToLeft(){return rightToLeft;};
  double *getQ(){return Q;};
};
#endif //__STEREOPARAM_H__
