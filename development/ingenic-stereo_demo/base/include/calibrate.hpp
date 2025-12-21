#ifndef __CALIBRATE_HPP__
#define __CALIBRATE_HPP__

#include <opencv4/opencv2/core.hpp>
#include <opencv4/opencv2/imgcodecs.hpp>
#include <opencv4/opencv2/imgproc.hpp>
#include <opencv4/opencv2/calib3d.hpp>
#include <opencv4/opencv2/core.hpp>
#include <vector>

#ifdef OPENCV_GUI
#include "opencv2/highgui.hpp"
#endif

class camera_calibrate {
 public:
  std::vector< std::vector<cv::Point2f> > corner_points;
  std::vector< std::vector<cv::Point3f> > object_points;
  std::vector<cv::Point3f> object_buf;
  cv::Size board_shape;
  cv::Size image_size; // 640x480
  cv::Mat rmap_matrix[2];
  // (width, height) for square shape
  cv::Size square_measure;
  // (x, y) for grid end point coordinate.
  cv::Point2f grid_coordinate;

  cv::Mat camera_matrixl;
  cv::Mat dist_coeffl;
  cv::Mat rvecs;
  cv::Mat tvecs;
  int valid;
  int is_valid();
  camera_calibrate();
  ~camera_calibrate();

  void set_pattern(cv::Size _board_shape, cv::Size _image_size,
		   cv::Size _square_measure, cv::Point2f _grid_coordinate);
  bool get_corners_point(cv::Mat image, int flags);
  bool get_corners_point(std::string image_file, int flags);
  bool generate_object_points();
  void remove_last_corner_pair(void);
  bool calibrate(int flags);
  bool write_parameters(std::string intrinsic_file, cv::FileStorage::Mode=cv::FileStorage::WRITE,
                        std::string label_tag="");
  bool load_parameters(std::string intrinsic_file, cv::FileStorage::Mode=cv::FileStorage::READ,
                        std::string label_tag="");
  double get_rms();
  void clear();
 private:
  double RMS;
};

class stereo_calibrate {
 public:
  camera_calibrate camera_left;
  camera_calibrate camera_right;
  cv::Rect validRoi_left;
  cv::Rect validRoi_right;
  cv::Size image_size; // 640x480
  int valid;
  cv::Mat Q;
  cv::Mat T, R, E, F;
  cv::Mat Rl, Rr, Pl, Pr;
  cv::Mat perViewErrors;
  stereo_calibrate(cv::Size _board_shape, cv::Size _image_size,
		   cv::Size _square_measure, cv::Point2f _grid_coordinate);
  ~stereo_calibrate();
  void setup(cv::Size _board_shape, cv::Size _image_size,
		   cv::Size _square_measure, cv::Point2f _grid_coordinate);
  bool calibrate(cv::Mat image);
  bool calibrate();
  bool get_corners_point(cv::Mat imageL, cv::Mat imageR, int flags);
  bool get_corners_point(std::string  imageL, std::string imageR, int flags);
  bool calibrate(int flags);
  bool get_rectify_map();
  cv::Mat check_rectify(cv::Mat left, cv::Mat right, int show = 0);
  cv::Mat check_rectify(std::string left_file, std::string right_file, int show = 0);
  bool write_parameters(std::string intrinsic_file, std::string extrinsic_file, int flags=0);
  bool load_parameters(std::string intrinsic_file, std::string extrinsic_file, int flags=0);
  void clear();
  int is_valid();
  double get_rms();
 private:
  double RMS;
};

#endif
