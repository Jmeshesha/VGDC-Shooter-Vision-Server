#pragma once
#include <opencv2/core.hpp>
#include <string.h>
#include <opencv2/videoio.hpp>

namespace CameraMarkerServer {
class CalibrationSettings {
 public:
  CalibrationSettings() : goodInput(false) {}
  enum Pattern {
    NOT_EXISTING,
    CHESSBOARD,
    CHARUCOBOARD,
    CIRCLES_GRID,
    ASYMMETRIC_CIRCLES_GRID
  };
  enum InputType { INVALID, CAMERA, VIDEO_FILE, IMAGE_LIST };
  void write(cv::FileStorage& fs) const;
  void read(const cv::FileNode& node);
  void validate();
  cv::Mat nextImage();
  static bool readStringList(const std::string& filename,
                             std::vector<std::string>& l);
  static bool isListOfImages(const std::string& filename);


 public:
  cv::Size boardSize;  // The size of the board -> Number of items by width and
                       // height
  Pattern calibrationPattern;  // One of the Chessboard, ChArUco board, circles,
                               // or asymmetric circle pattern
  float calibrationSquareSize;  // The size of a square in your defined unit (point,
                     // millimeter,etc).
  float calibrationMarkerSize;  // The size of a marker in your defined unit
                                // (point,
                     // millimeter,etc).
  float poseMarkerSize;
  int windowSize; 
  std::string arucoDictName;  // The Name of ArUco dictionary which you use in
                              // ChArUco pattern
  std::string arucoDictFileName;  // The Name of file which contains ArUco
                                  // dictionary for ChArUco pattern
  int nrFrames;  // The number of frames to use from the input for calibration
  float aspectRatio;            // The aspect ratio
  int delay;                    // In case of a video input
  bool writePoints;             // Write detected feature points
  bool writeExtrinsics;         // Write extrinsic parameters
  bool writeGrid;               // Write refined 3D target grid points
  bool calibZeroTangentDist;    // Assume zero tangential distortion
  bool calibFixPrincipalPoint;  // Fix the principal point at the center
  bool flipVertical;  // Flip the captured images around the horizontal axis
  std::string outputFileName;  // The name of the file where to write
  bool showUndistorted;        // Show undistorted images after calibration
  std::string input;           // The input ->
  bool useFisheye;             // use fisheye camera model for calibration
  bool fixK1;                  // fix K1 distortion coefficient
  bool fixK2;                  // fix K2 distortion coefficient
  bool fixK3;                  // fix K3 distortion coefficient
  bool fixK4;                  // fix K4 distortion coefficient
  bool fixK5;                  // fix K5 distortion coefficient

  int cameraID;
  std::vector<std::string> imageList;
  size_t atImageList;
  cv::VideoCapture inputCapture;
  InputType inputType;
  bool goodInput;
  int flag;

 private:
  std::string patternToUse;
};
static inline void read(
    const cv::FileNode& node, CalibrationSettings& x,
    const CalibrationSettings& default_value = CalibrationSettings()) {
  if (node.empty())
    x = default_value;
  else
    x.read(node);
}
}  // namespace CameraMarkerServer