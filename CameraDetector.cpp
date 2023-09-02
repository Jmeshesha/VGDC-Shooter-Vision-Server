
#include "CameraDetector.h"
#include <iostream>
#include <stdio.h>
#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/videoio.hpp>
#include <opencv2/calib3d.hpp>
#include <optional>

namespace CameraMarkerServer {
   
std::optional<Pose> PoseDetector::DetectPose(cv::Mat camera_frame, const bool draw_detected_image) {
  std::vector<int> ids;
  std::vector<std::vector<cv::Point2f>> corners;


  if (camera_frame.empty()) {
    return std::nullopt;
  }
  


  

  
  std::optional<Pose> answer = std::nullopt;
  cv::Mat obj_points(4, 1, CV_32FC3);
  obj_points.ptr<cv::Vec3f>(0)[0] =
      cv::Vec3f(-marker_length_ / 2.f, marker_length_ / 2.f, 0);
  obj_points.ptr<cv::Vec3f>(0)[1] =
      cv::Vec3f(marker_length_ / 2.f, marker_length_ / 2.f, 0);
  obj_points.ptr<cv::Vec3f>(0)[2] =
      cv::Vec3f(marker_length_ / 2.f, -marker_length_ / 2.f, 0);
  obj_points.ptr<cv::Vec3f>(0)[3] =
      cv::Vec3f(-marker_length_ / 2.f, -marker_length_ / 2.f, 0);
  aruco_detector_.detectMarkers(camera_frame, corners, ids);
  if (ids.size() > 0) {
    int id = ids.front();
    std::vector<cv::Point2f> frame_corners = corners.front();
    Pose detected_pose;
    std::cout << "Found marker!" << std::endl;
    cv::Mat rvec;
    cv::Mat tvec;

    if (cv::solvePnP(obj_points, corners.front(),
                      camera_parameters_.insintric_camera_parms,
                      camera_parameters_.distortion_mat, rvec,
                      tvec)) {
      detected_pose.translation = tvec;
      cv::Mat rot_mat;

      cv::Rodrigues(rvec, rot_mat);
      rot_mat.col(1).copyTo(detected_pose.up);
      rot_mat.col(2).copyTo(detected_pose.forward);
      cv::Mat drawnMarkers;
      camera_frame.copyTo(drawnMarkers);
      cv::drawFrameAxes(drawnMarkers, camera_parameters_.insintric_camera_parms,
                        camera_parameters_.distortion_mat, rvec, tvec, marker_length_);
      cv::imshow("CameraServer", drawnMarkers);
      return detected_pose;
    }
  }

   cv::imshow("CameraServer", camera_frame);
  

  
  return answer;
}

}

