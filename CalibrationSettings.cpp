#include "CalibrationSettings.h"
#include <iostream>
#include <sstream>
#include <string>
#include <ctime>
#include <cstdio>


#include <opencv2/core.hpp>
#include <opencv2/core/utility.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/calib3d.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/videoio.hpp>
#include <opencv2/highgui.hpp>
#include "opencv2/objdetect/charuco_detector.hpp"


namespace CameraMarkerServer {
void CalibrationSettings::write(cv::FileStorage& fs) const  // Write serialization for this class
{
  fs << "{"
     << "BoardSize_Width" << boardSize.width << "BoardSize_Height"
     << boardSize.height << "Calibration_Square_Size" << calibrationSquareSize
     << "Calibration_Marker_Size" << calibrationMarkerSize << "Pose_Marker_Size"
     << poseMarkerSize << "Window_Size" << windowSize
     << "Calibrate_Pattern" << patternToUse << "ArUco_Dict_Name"
     << arucoDictName << "ArUco_Dict_File_Name" << arucoDictFileName
     << "Calibrate_NrOfFrameToUse" << nrFrames << "Calibrate_FixAspectRatio"
     << aspectRatio << "Calibrate_AssumeZeroTangentialDistortion"
     << calibZeroTangentDist << "Calibrate_FixPrincipalPointAtTheCenter"
     << calibFixPrincipalPoint

     << "Write_DetectedFeaturePoints" << writePoints
     << "Write_extrinsicParameters" << writeExtrinsics << "Write_gridPoints"
     << writeGrid << "Write_outputFileName" << outputFileName 
     << "Show_UndistortedImage" << showUndistorted

     << "Input_FlipAroundHorizontalAxis" << flipVertical << "Input_Delay"
     << delay << "Input" << input << "}";

}

void CalibrationSettings::read(const cv::FileNode& node)  // Read serialization for this class
{
  node["BoardSize_Width"] >> boardSize.width;
  node["BoardSize_Height"] >> boardSize.height;
  node["Window_Size"] >> windowSize;
  node["Calibrate_Pattern"] >> patternToUse;
  node["ArUco_Dict_Name"] >> arucoDictName;
  node["ArUco_Dict_File_Name"] >> arucoDictFileName;
  node["Calibration_Square_Size"] >> calibrationSquareSize;
  node["Calibration_Marker_Size"] >> calibrationMarkerSize;
  node["Pose_Marker_Size"] >> poseMarkerSize;
  node["Calibrate_NrOfFrameToUse"] >> nrFrames;
  node["Calibrate_FixAspectRatio"] >> aspectRatio;
  node["Write_DetectedFeaturePoints"] >> writePoints;
  node["Write_extrinsicParameters"] >> writeExtrinsics;
  node["Write_gridPoints"] >> writeGrid;
  node["Write_outputFileName"] >> outputFileName;
  node["Calibrate_AssumeZeroTangentialDistortion"] >> calibZeroTangentDist;
  node["Calibrate_FixPrincipalPointAtTheCenter"] >> calibFixPrincipalPoint;
  node["Calibrate_UseFisheyeModel"] >> useFisheye;
  node["Input_FlipAroundHorizontalAxis"] >> flipVertical;
  node["Show_UndistortedImage"] >> showUndistorted;
  node["Input"] >> input;
  node["Input_Delay"] >> delay;
  node["Fix_K1"] >> fixK1;
  node["Fix_K2"] >> fixK2;
  node["Fix_K3"] >> fixK3;
  node["Fix_K4"] >> fixK4;
  node["Fix_K5"] >> fixK5;

  validate();
}

void CalibrationSettings::validate() {
  goodInput = true;
  if (boardSize.width <= 0 || boardSize.height <= 0) {
    std::cerr << "Invalid Board size: " << boardSize.width << " "
              << boardSize.height << std::endl;
    goodInput = false;
  }
  if (calibrationSquareSize <= 10e-6) {
    std::cerr << "Invalid calibration square size " << calibrationSquareSize << std::endl;
    goodInput = false;
  }
  if (nrFrames <= 0) {
    std::cerr << "Invalid number of frames " << nrFrames << std::endl;
    goodInput = false;
  }
  if (poseMarkerSize <= 10e-6) {
    std::cerr << "Invalid pose marker size " << poseMarkerSize << std::endl;
  }
  if (windowSize <= 0) {
    std::cerr << "Invalid window size " << windowSize << std::endl;
    goodInput = false;
  }
  if (input.empty())  // Check for valid input
    inputType = INVALID;
  else {
    if (input[0] >= '0' && input[0] <= '9') {
      std::stringstream ss(input);
      ss >> cameraID;
      inputType = CAMERA;
    } else {
      if (isListOfImages(input) && readStringList(input, imageList)) {
        inputType = IMAGE_LIST;
        nrFrames = (nrFrames < (int)imageList.size()) ? nrFrames
                                                      : (int)imageList.size();
      } else
        inputType = VIDEO_FILE;
    }
    if (inputType == CAMERA) inputCapture.open(cameraID);
    if (inputType == VIDEO_FILE) inputCapture.open(input);
    if (inputType != IMAGE_LIST && !inputCapture.isOpened())
      inputType = INVALID;
  }
  if (inputType == INVALID) {
    std::cerr << " Input does not exist: " << input;
    goodInput = false;
  }

  flag = 0;
  if (calibFixPrincipalPoint) flag |= cv::CALIB_FIX_PRINCIPAL_POINT;
  if (calibZeroTangentDist) flag |= cv::CALIB_ZERO_TANGENT_DIST;
  if (aspectRatio) flag |= cv::CALIB_FIX_ASPECT_RATIO;
  if (fixK2) flag |= cv::CALIB_FIX_K2;
  if (fixK3) flag |= cv::CALIB_FIX_K3;
  if (fixK1) flag |= cv::CALIB_FIX_K1;
  if (fixK4) flag |= cv::CALIB_FIX_K4;
  if (fixK5) flag |= cv::CALIB_FIX_K5;

  if (useFisheye) {
    // the fisheye model has its own enum, so overwrite the flags
    flag = cv::fisheye::CALIB_FIX_SKEW | cv::fisheye::CALIB_RECOMPUTE_EXTRINSIC;
    if (fixK1) flag |= cv::fisheye::CALIB_FIX_K1;
    if (fixK2) flag |= cv::fisheye::CALIB_FIX_K2;
    if (fixK3) flag |= cv::fisheye::CALIB_FIX_K3;
    if (fixK4) flag |= cv::fisheye::CALIB_FIX_K4;
    if (calibFixPrincipalPoint) flag |= cv::fisheye::CALIB_FIX_PRINCIPAL_POINT;
  }

  calibrationPattern = NOT_EXISTING;
  if (!patternToUse.compare("CHESSBOARD")) calibrationPattern = CHESSBOARD;
  if (!patternToUse.compare("CHARUCOBOARD")) calibrationPattern = CHARUCOBOARD;
  if (!patternToUse.compare("CIRCLES_GRID")) calibrationPattern = CIRCLES_GRID;
  if (!patternToUse.compare("ASYMMETRIC_CIRCLES_GRID"))
    calibrationPattern = ASYMMETRIC_CIRCLES_GRID;
  if (calibrationPattern == NOT_EXISTING) {
    std::cerr << " Camera calibration mode does not exist: " << patternToUse
              << std::endl;
    goodInput = false;
  }
  atImageList = 0;
}

cv::Mat CalibrationSettings::nextImage() {
  cv::Mat result;
  if (inputCapture.isOpened()) {
    cv::Mat view0;
    inputCapture >> view0;

    view0.copyTo(result);
  } else if (atImageList < imageList.size())
    result = cv::imread(imageList[atImageList++], cv::IMREAD_COLOR);

  return result;
}

// static
bool CalibrationSettings::readStringList(const std::string& filename,
                           std::vector<std::string>& l) {
  l.clear();
  cv::FileStorage fs(filename, cv::FileStorage::READ);
  if (!fs.isOpened()) return false;
  cv::FileNode n = fs.getFirstTopLevelNode();
  if (n.type() != cv::FileNode::SEQ) return false;
  cv::FileNodeIterator it = n.begin(), it_end = n.end();
  for (; it != it_end; ++it) l.push_back((std::string)*it);
  return true;
}

// static
bool CalibrationSettings::isListOfImages(const std::string& filename) {
  std::string s(filename);
  // Look for file extension
  if (s.find(".xml") == std::string::npos &&
      s.find(".yaml") == std::string::npos &&
      s.find(".yml") == std::string::npos)
    return false;
  else
    return true;
}
}  // namespace CameraMarkerServer