#include "calibrate.hpp"
#include <iostream>
#include <test.hpp>

using namespace std;
using namespace cv;


stereo_calibrate::stereo_calibrate(Size _board_shape, Size _image_size,
				   Size _square_measure, Point2f _grid_coordinate)
{
    setup(_board_shape, _image_size, _square_measure, _grid_coordinate);
}


void stereo_calibrate::setup(Size _board_shape, Size _image_size,
				   Size _square_measure, Point2f _grid_coordinate)
{
  camera_left.set_pattern(_board_shape, _image_size, _square_measure, _grid_coordinate);
  camera_right.set_pattern(_board_shape, _image_size, _square_measure, _grid_coordinate);
  image_size = _image_size;
  valid = 0;
  RMS = -1;
}

stereo_calibrate::~stereo_calibrate()
{
}

bool stereo_calibrate::get_corners_point(Mat imageL, Mat imageR, int flags)
{
  bool ret_left = camera_left.get_corners_point(imageL, flags);
  if (!ret_left)
    return false;

  bool ret_right = camera_right.get_corners_point(imageR, flags);

  if (!ret_right)
    {
      camera_left.remove_last_corner_pair();
      return false;
    }
  return true;
}

bool stereo_calibrate::get_corners_point(string imageL, string imageR, int flags)
{
  bool ret_left = camera_left.get_corners_point(imageL, flags);
  if (!ret_left)
    return false;

  bool ret_right = camera_right.get_corners_point(imageR, flags);

  if (!ret_right)
    {
      camera_left.remove_last_corner_pair();
      return false;
    }
  return true;
}

bool stereo_calibrate::calibrate(int flags)
{
  long long tmp_time;

  tmp_time = GetMicrosecondCount();

  // camera calibrate
  if (!(camera_left.calibrate(flags) && camera_right.calibrate(flags)))
    {
      valid = 0;
      return false;
    }

  tmp_time = GetMicrosecondCount() - tmp_time;

  cout << "[time] stereo - camera calibrate time " << tmp_time << endl;
  tmp_time = GetMicrosecondCount();

  // stereo calibrate use fixed camera intrinsic
  RMS = stereoCalibrate(camera_left.object_points, camera_left.corner_points,
			       camera_right.corner_points,
			       camera_left.camera_matrixl, camera_left.dist_coeffl,
			       camera_right.camera_matrixl, camera_right.dist_coeffl,
			       camera_left.image_size, R, T, E, F, perViewErrors,
			       CALIB_FIX_INTRINSIC + CALIB_RATIONAL_MODEL ,
			       TermCriteria(TermCriteria::COUNT+TermCriteria::EPS, 100, 1e-5));

  if (R.empty() || T.empty())
    {
      valid = 0;
      return false;
    }

  tmp_time = GetMicrosecondCount() - tmp_time;
  cout << "[time] stereo calibrate time " << tmp_time << endl;

  cout << "done with RMS error=" << RMS <<  endl;

  stereoRectify(camera_left.camera_matrixl, camera_left.dist_coeffl,
		camera_right.camera_matrixl, camera_right.dist_coeffl,
		camera_left.image_size, R, T, Rl, Rr, Pl, Pr, Q,
		CALIB_ZERO_DISPARITY, 0, Size(0,0), &validRoi_left, &validRoi_right);
  valid = 1;
  return true;
}

bool stereo_calibrate::get_rectify_map()
{
  camera_calibrate *tcam = &camera_left;

  initUndistortRectifyMap(tcam->camera_matrixl, tcam->dist_coeffl, Rl, Pl, image_size,
			  CV_16SC2, tcam->rmap_matrix[0], tcam->rmap_matrix[1]);

  tcam = &camera_right;
  initUndistortRectifyMap(tcam->camera_matrixl, tcam->dist_coeffl, Rl, Pl, image_size,
			  CV_16SC2, tcam->rmap_matrix[0], tcam->rmap_matrix[1]);

  return true;
}

Mat stereo_calibrate::check_rectify(Mat left, Mat right, int show)
{
  Mat mapped_left, mapped_right;

  if (!camera_left.is_valid() || !camera_right.is_valid())
    {
      cout << "Not calirated camera. please calirate first" << endl;
      return Mat();
    }

  if (camera_left.rmap_matrix[0].empty() || camera_right.rmap_matrix[0].empty())
    {
      get_rectify_map();
    }

  remap(left, mapped_left, camera_left.rmap_matrix[0], camera_left.rmap_matrix[1], INTER_LINEAR);
  remap(right, mapped_right, camera_right.rmap_matrix[0], camera_right.rmap_matrix[1], INTER_LINEAR);
  double sf = 1.0f;

  int w = image_size.width, h = image_size.height;

  Mat canvas;

  if (left.type() == CV_8UC1)
    canvas.create(h, w*2, CV_8UC1);
  else
    {
      cout << "invalid image type " << endl;
      return canvas;
    }

  Mat canvas_left = canvas(Rect(0, 0, w , h));
  Mat canvas_right = canvas(Rect(w, 0, w, h));

  resize(left, canvas_left, canvas_left.size(), 0, 0, INTER_AREA);
  resize(right, canvas_right, canvas_right.size(), 0, 0, INTER_AREA);

  Rect vroi(cvRound(validRoi_left.x * sf), cvRound(validRoi_left.y * sf),
	    cvRound(validRoi_left.width * sf), cvRound(validRoi_left.height * sf));

  rectangle(canvas_left, vroi, Scalar(0,0,255), 3, 8);

  vroi = Rect(cvRound(validRoi_right.x*sf), cvRound(validRoi_right.y*sf),
	      cvRound(validRoi_right.width*sf), cvRound(validRoi_right.height*sf));

  rectangle(canvas_right, vroi, Scalar(0,0,255), 3, 8);

  for (int j = 0; j < canvas.rows; j += 16)
    line (canvas, Point(0,j), Point(canvas.cols, j), Scalar(0, 255, 0), 1, 8);

#ifdef OPENCV_GUI
  if (show)
    {
      namedWindow("rectified", 1);
      imshow("rectified", canvas);
      while((char)waitKey());
      destroyAllWindows();
    }
#endif

  return canvas;
}

cv::Mat stereo_calibrate::check_rectify(string left_file, string right_file, int show)
{
  Mat left = imread(left_file, cv::IMREAD_GRAYSCALE);
  Mat right = imread(right_file, cv::IMREAD_GRAYSCALE);

  if (left.empty() || right.empty())
    {
      cout << "open file " << left_file << " or " << right_file << " fail!" << endl;
      return Mat();
    }

  return check_rectify(left, right, show);
}

bool stereo_calibrate::write_parameters(string intrinsic_file, string extrinsic_file, int flags)
{
    if (!is_valid())
        return false;

    camera_left.write_parameters(intrinsic_file, FileStorage::WRITE,  "_left");
    camera_right.write_parameters(intrinsic_file, FileStorage::APPEND, "_right");

    FileStorage ext_fd(extrinsic_file, FileStorage::WRITE);
    if (ext_fd.isOpened()) {
        ext_fd << "Q" << Q;
        ext_fd << "T" << T;
        ext_fd << "Rl" << Rl;
        ext_fd << "Pl" << Pl;
        ext_fd << "Rr" << Rr;
        ext_fd << "Pr" << Pr;
        if (flags) {
            ext_fd << "R" << R;
            ext_fd << "E" << E;
            ext_fd << "F" << F;
        }
    } else {
        cout << "open and write extrinsic file: \"" << extrinsic_file << "\" fail!" << endl;
    }
    return true;
}

bool stereo_calibrate::load_parameters(string intrinsic_file, string extrinsic_file, int flags)
{
    bool left_isok = camera_left.load_parameters(intrinsic_file,FileStorage::READ,"_left");
    bool right_isok = camera_right.load_parameters(intrinsic_file,FileStorage::READ,"_right");
    FileStorage ext_fd(extrinsic_file,FileStorage::READ);
    if(ext_fd.isOpened()) {
        ext_fd["Q"] >> Q;
        ext_fd["T"] >> T;
        ext_fd["Rl"] >> Rl;
        ext_fd["Pl"] >> Pl;
        ext_fd["Rr"] >> Rr;
        ext_fd["Pr"] >> Pr;
        if (Q.empty() || T.empty() || Rl.empty() || Pl.empty() || Rr.empty() || Pr.empty()){
            valid = 0;
            return false;
        }

        if(flags) {
            ext_fd["R"] >> R;
            ext_fd["E"] >> E;
            ext_fd["F"] >> F;
            if(R.empty() || E.empty() || F.empty()) {
                valid = 0;
                return false;
            }
        }
        if (left_isok && right_isok)
            valid = 1;
    }else {
        cout << "open and read extrinsic file: \"" << extrinsic_file << "\" fail!" << endl;
        valid = 0;
        return false;
    }
    return true;
}

int stereo_calibrate::is_valid()
{
  return valid;
}

void stereo_calibrate::clear()
{
  camera_left.clear();
  camera_right.clear();

  // should clean all camera matrix
  RMS = -1.;
  valid = 0;
}

double stereo_calibrate::get_rms()
{
    return RMS;
}
