#ifndef CAMERA_DETECTOR_UTILS_H_
#define CAMERA_DETECTOR_UTILS_H_
#include <opencv2/core.hpp>
#include <opencv2/core/types.hpp>
#include <opencv2/videoio.hpp>
#include "CameraCalibratationUtils.h"
#include <opencv2/aruco.hpp>
#include <opencv2/calib3d/calib3d.hpp>

namespace CameraMarkerServer {
struct Pose {
  cv::Vec3d forward;
  cv::Vec3d up;
  cv::Vec3d translation;
};

class PoseDetector {
 public:
  PoseDetector(float marker_length, 
               cv::aruco::Dictionary dictionary,
               cv::aruco::DetectorParameters detection_params,
               const CameraParameters calibration_params)
      : aruco_detector_(dictionary, detection_params),
        camera_parameters_(calibration_params),
        marker_length_(marker_length) {}
  std::optional<Pose> DetectPose(const cv::Mat camera_frame,
                                 const bool draw_detected_image);

 private:
  cv::aruco::ArucoDetector aruco_detector_;
  const CameraParameters camera_parameters_;
  float marker_length_;

};


	
}  // namespace CameraMarkerServer
#endif     // CAMERA_DETECTOR_UTILS_H_CAMERA_DETECTOR_UTILS_H_