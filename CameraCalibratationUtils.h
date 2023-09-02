#pragma once
#include <optional>
#include <string>
#include <opencv2/core.hpp>
#include <opencv2/aruco.hpp>
#include "CalibrationSettings.h"
namespace CameraMarkerServer {
struct CameraParameters {
  cv::Mat insintric_camera_parms;
  cv::Mat distortion_mat;
};

std::optional<CameraParameters> CalibrateAndSaveCameraParameters(
    CalibrationSettings &camera_settings);


std::optional<CameraParameters> GetCameraParametersFromFile(
    CalibrationSettings &camera_settings);

std::optional<const CameraParameters> CalulateCameraParameters(
    CalibrationSettings &camera_settings);

std::optional<cv::aruco::Dictionary> CreateArucoDict(CalibrationSettings &s);




}  // namespace CameraMarkerServer