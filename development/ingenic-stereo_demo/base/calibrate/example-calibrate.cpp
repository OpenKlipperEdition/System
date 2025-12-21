#include "calibrate.hpp"
#include <iostream>
#include <stdio.h>
using namespace std;
using namespace cv;

static bool parser_file_list( const string& filename, vector<string>&l, vector<string>&r)
{
  l.resize(0);
  r.resize(0);
  FileStorage fs(filename, FileStorage::READ);
  if( !fs.isOpened() )
    {
      printf("xml file open fail\n");
      return false;
    }

  FileNode n = fs.getFirstTopLevelNode();
  if( n.type() != FileNode::SEQ )
      return false;

  FileNodeIterator it = n.begin(), it_end = n.end();

  for( ; it != it_end; ++it )
    {
      cout << "left: " << (string)*it << endl;
      l.push_back((string)*(it++));
      cout << "right: " << (string)*it << endl;
      r.push_back((string)*it);
    }
  return true;
}

void stage_help()
{
  cout << "list: --list=<list filenmae>\n" ;
  exit(0);
}

class xtest{
public:
  int x;
  xtest(){  cout << "call cor no x" << this << endl ;}
  xtest(int _x){ x = _x ; cout << "call cor " << this << endl ;}
  ~xtest(){cout << "call decore " << this << endl;}
};

int main(int argc, char *argv[])
{
  Size board_shape(6, 4);
  Size image_size(640, 480);
  Size square_measure(27.5, 27.5);
  Point2f low_right_coord (137.5, 83.2);
  cv::CommandLineParser parser(argc, argv, "{list||}{help h||}");

  if(parser.has("help"))
      stage_help();

  string xml_file = parser.get<string>("list");
  if (xml_file.empty())
    {
      cout << "missing xml file" << endl;
      exit(-1);
    }

  stereo_calibrate stereo_calib(board_shape, image_size, square_measure, low_right_coord);

  vector<string> good_image_pair;
  vector<string> left_image_list, right_image_list;

  parser_file_list(xml_file, left_image_list, right_image_list);

  for(int i = 0; i < left_image_list.size(); i++)
    {
      string left = left_image_list[i];
      string right = right_image_list[i];
      bool ret  = stereo_calib.get_corners_point(left, right, 0);

      if(ret) {
	  good_image_pair.push_back(left);
	  cout << "good image: ";
      } else {
	  cout << "invalid image: ";
      }
      printf("left %s right %s\n", left.c_str(), right.c_str());
    }

  stereo_calib.calibrate(1);

  stereo_calib.check_rectify(left_image_list[0], right_image_list[0]);
  stereo_calib.write_parameters("ins.json", "ext.json", 1);
  return 0;
}
