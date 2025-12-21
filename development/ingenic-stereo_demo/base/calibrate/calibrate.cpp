#include "calibrate.hpp"
#include <iostream>
#ifdef OPENCV_GUI
#include <opencv2/highgui.hpp>
#endif
#include <test.hpp>
using namespace std;
using namespace cv;


void camera_calibrate::set_pattern(Size _board_shape, Size _image_size,
			      Size _square_measure, Point2f _grid_coordinate)
{
    clear();
    board_shape = _board_shape;
    image_size = _image_size;
    square_measure = _square_measure;
    grid_coordinate = _grid_coordinate;

    object_buf.clear();
    for (int i = 0; i < board_shape.height; i++)
        for (int j = 0; j < board_shape.width; j++) {
            object_buf.push_back(Point3f(j * square_measure.width, i * square_measure.height, 0));
        }

    object_buf[board_shape.width - 1].x = object_buf[0].x + grid_coordinate.x;
    valid = 0;
}


camera_calibrate::camera_calibrate()
{
  valid = 0;
  RMS = -1;
}

camera_calibrate::~camera_calibrate()
{
}

static void showimage(Mat image)
{
#ifdef OPENCV_GUI
  namedWindow("Display window", 0 );
  resizeWindow("Display window", 500, 500);
  imshow("Display window", image);
  waitKey(0);
  destroyWindow("Display window");
#endif
}

bool camera_calibrate::get_corners_point(Mat image, int flags)
{
    vector<Point2f> point_buf;
    bool found;

    if (image.empty()) {
        cout << "invalid image" << endl;
        return false;
    }

    if (image.type() != CV_8UC1) {
        if (image.type() == CV_8UC3) {
            Mat tmp;
            cvtColor(image, tmp, COLOR_BGR2GRAY);
            image = tmp;
        } else {
            cout << "invalid image type " << endl;
            return false;
        }
    }

    found = findChessboardCorners(image, board_shape, point_buf, CALIB_CB_ADAPTIVE_THRESH
            | CALIB_CB_FAST_CHECK | CALIB_CB_NORMALIZE_IMAGE);
    if (!found)
        return false;

    cornerSubPix(image, point_buf, Size(10, 10),
            Size(-1,-1), TermCriteria( TermCriteria::EPS+TermCriteria::COUNT, 30, 0.0001 ));

    corner_points.push_back(point_buf);
    object_points.push_back(object_buf);

    drawChessboardCorners(image, board_shape, Mat(point_buf), found);
    if (flags)
        showimage(image);

    return true;
}

void camera_calibrate::remove_last_corner_pair()
{
    object_points.pop_back();
    corner_points.pop_back();
}

bool camera_calibrate::get_corners_point(string image_file, int flags)
{
  vector<Point2f> point_buf;

  Mat image;
  image = imread(image_file, cv::IMREAD_GRAYSCALE);
  // image = imread(image_file, 1);
  if (image.empty()) {
      cout << "open image " << image_file << "fail" << endl;
      return false;
  }
  return get_corners_point(image, flags);
}

#define RECOMPUTE  1
bool camera_calibrate::calibrate(int flags)
{
  if (!camera_matrixl.empty() && !dist_coeffl.empty() && flags != RECOMPUTE)
    {
      valid = 1;
      return true;
    }

  /*  if give grid size, then use release object */
  bool release_object = true;
  int iFixedPoint = -1;
  iFixedPoint = release_object ? (board_shape.width - 1) : iFixedPoint;

  vector<Point3f> new_object_points;

  Mat rvecs, tvecs;
  long long tmp_time;
  tmp_time = GetMicrosecondCount();

  if (object_points.size() <= 0 || corner_points.size() <= 0) {
      valid = 0;
      return false;
  }

  RMS = calibrateCameraRO(object_points, corner_points, image_size, board_shape.width - 1,
			  camera_matrixl, dist_coeffl, rvecs, tvecs, new_object_points,
			  CALIB_RATIONAL_MODEL |CALIB_USE_LU);

  tmp_time = GetMicrosecondCount() - tmp_time;
  cout << "[time] camera calibrate time " << tmp_time << endl;

  if (rvecs.empty() || tvecs.empty() || camera_matrixl.empty() || dist_coeffl.empty())
    {
      valid = 0;
      printf("cannot not get rvecs or tvecs\n");
      return false;
    }

  printf("RMS error reported by calibrateCamera: %g\n",RMS );
  valid = 1;
  return true;
}

bool camera_calibrate::write_parameters(string intrinsic_file, enum cv::FileStorage::Mode mode,
					string label_tag)
{
  if (!is_valid())
    return false;

  FileStorage ins_fd(intrinsic_file, mode);
  if (ins_fd.isOpened()) {
      ins_fd << "camera_matrix" + label_tag << camera_matrixl;
      ins_fd << "dist_coeff" + label_tag <<  dist_coeffl;
      ins_fd << "rvecs" + label_tag << rvecs;
      ins_fd << "tvecs" + label_tag << tvecs;
    } else {
      cout << "open and write intrinsic file: \"" << intrinsic_file << "\" fail!" << endl;
    }

  return true;
}

bool camera_calibrate::load_parameters(string intrinsic_file, enum cv::FileStorage::Mode mode,
        string label_tag)
{
    FileStorage ins_fd(intrinsic_file, mode);
    if (ins_fd.isOpened()) {
        ins_fd["camera_matrix" + label_tag] >> camera_matrixl;
        ins_fd["dist_coeff" + label_tag] >> dist_coeffl;
        ins_fd["rvecs" + label_tag] >> rvecs;
        ins_fd["tvecs" + label_tag] >> tvecs;
        if (camera_matrixl.empty() || dist_coeffl.empty() || rvecs.empty() || tvecs.empty()) {
            valid = 0;
            return false;
        } else
            valid = 1;
    } else {
        cout << "open and read intrinsic file: \"" << intrinsic_file << "\" fail!" << endl;
        valid = 0;
        return false;
    }
    return true;
}

int camera_calibrate::is_valid()
{
  return valid;
}

double camera_calibrate::get_rms()
{
    return RMS;
}

void camera_calibrate::clear()
{
  for (vector<Point2f> &elem: corner_points)
    elem.clear();
  corner_points.clear();

  for (vector<Point3f> &elem: object_points)
    elem.clear();
  object_points.clear();

  RMS = -1;
  valid = 0;
}
