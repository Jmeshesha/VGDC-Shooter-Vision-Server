#include "Client.h"

#include <iostream>
#include <asio/io_service.hpp>
#include <chrono>
#include <thread>
#include <optional>
#include "CalibrationSettings.h"
#include "CameraCalibratationUtils.h"
#include "CameraDetector.h"
#include <opencv2/core.hpp>
#include <opencv2/core/utility.hpp>
#include <opencv2/videoio.hpp>
#include <opencv2/highgui.hpp>
namespace CameraMarkerServer {
const std::string ADDRESS = "localhost";
const std::string PORT = "7777";
const std::string CALIBRATION_SETTINGS_FILE = "Calibration/calibration_settings.xml";
namespace {


std::optional<CalibrationSettings> ReadCalibrationSettings(
    const std::string settings_file_path) {
    CalibrationSettings s;
    cv::FileStorage fs(settings_file_path,
                            cv::FileStorage::READ);  // Read the settings
    if (!fs.isOpened()) {
        std::cout << "Could not open the configuration file: \""
                    << settings_file_path << "\"" << std::endl;
        return std::nullopt;
    }
    try {
        fs["Settings"] >> s;
    } catch (...) {
        std::cout << "Invalid server settings file" << std::endl;
        return std::nullopt;
    }
      
    fs.release();  // close Settings file
    return s;
}
}

bool Client::SetupSocket() {}

void Client::Run() {
  isRunning = true;
  asio::io_service io_service;
  UDPClient client(io_service);
  if (!client.OpenConnection(ADDRESS, PORT)) {
    return;
  }
  std::cout << "Loading Server settings..." << std::endl;
  std::optional<CalibrationSettings> optional_settings =
      ReadCalibrationSettings(CALIBRATION_SETTINGS_FILE);
  if (!optional_settings.has_value()) {
    return;
  }
  CalibrationSettings camera_settings = optional_settings.value();
  if (camera_settings.inputType != CalibrationSettings::InputType::CAMERA) {
    std::cout << "invalid input type. Only camera is supported." << std::endl;
    return;
  }
  std::optional<cv::aruco::Dictionary> dictionary =
      CreateArucoDict(camera_settings);
  if (!dictionary.has_value()) {
    std::cout << "Could not parse aruco dictionary." << std::endl;
    return;
  }
  std::cout << "Server settings successfully loaded!" << std::endl;
  std::cout << "Calibrating Camera..." << std::endl;
  std::optional<CameraParameters> camera_params =
      CalulateCameraParameters(camera_settings);
  if (!camera_params.has_value()) {
    std::cout << "Could not calculate camera calibrations values exiting server"
              << std::endl;
    return;
  }

  std::cout << "Camera successfully calibrated!" << std::endl;
 
  PoseDetector detector(camera_settings.poseMarkerSize,
                        dictionary.value(),
                        cv::aruco::DetectorParameters(), camera_params.value());
  const char ESC_KEY = 27;
  std::cout << "Running pose estimation..." << std::endl;
  
  while (isRunning) {
    cv::Mat frame;
    camera_settings.inputCapture.read(frame);

    std::optional<Pose> pose_optional = detector.DetectPose(frame, true);
    if (pose_optional.has_value()) {
      Pose pose = pose_optional.value();
      std::ostringstream os;
      os << pose.forward << "_" << pose.up << "_" << pose.translation;
      client.Send(os.str());
      std::cout << os.str() << std::endl;
    }
    char key = cv::waitKey(camera_settings.delay);
    if (key == ESC_KEY) {
      isRunning = false;
    }
    
    //std::cout << "Hello!" << std::endl;
  }
}
}  // namespace CameraMarkerServer
