#define _CRT_SECURE_NO_WARNINGS

#include "CameraCalibratationUtils.h"
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

namespace {



enum { DETECTION = 0, CAPTURING = 1, CALIBRATED = 2 };

bool runCalibrationAndSave(CalibrationSettings& s, cv::Size imageSize,
                           cv::Mat& cameraMatrix, cv::Mat& distCoeffs,
                           std::vector<std::vector<cv::Point2f> > imagePoints,
                           float grid_width, bool release_object);

//! [compute_errors]
static double computeReprojectionErrors(
    const std::vector<std::vector<cv::Point3f> >& objectPoints,
    const std::vector<std::vector<cv::Point2f> >& imagePoints, const std::vector<cv::Mat>& rvecs,
    const std::vector<cv::Mat>& tvecs, const cv::Mat& cameraMatrix, const cv::Mat& distCoeffs,
    std::vector<float>& perViewErrors, bool fisheye) {
  std::vector<cv::Point2f> imagePoints2;
  size_t totalPoints = 0;
  double totalErr = 0, err;
  perViewErrors.resize(objectPoints.size());

  for (size_t i = 0; i < objectPoints.size(); ++i) {
    if (fisheye) {
      cv::fisheye::projectPoints(objectPoints[i], imagePoints2, rvecs[i], tvecs[i],
                             cameraMatrix, distCoeffs);
    } else {
      projectPoints(objectPoints[i], rvecs[i], tvecs[i], cameraMatrix,
                    distCoeffs, imagePoints2);
    }
    err = norm(imagePoints[i], imagePoints2, cv::NORM_L2);

    size_t n = objectPoints[i].size();
    perViewErrors[i] = (float)std::sqrt(err * err / n);
    totalErr += err * err;
    totalPoints += n;
  }

  return std::sqrt(totalErr / totalPoints);
}
//! [compute_errors]
//! [board_corners]
static void calcBoardCornerPositions(
    cv::Size boardSize, float squareSize, std::vector<cv::Point3f>& corners,
    CalibrationSettings::Pattern patternType /*= Settings::CHESSBOARD*/) {
  corners.clear();

  switch (patternType) {
    case CalibrationSettings::CHESSBOARD:
    case CalibrationSettings::CIRCLES_GRID:
      for (int i = 0; i < boardSize.height; ++i) {
        for (int j = 0; j < boardSize.width; ++j) {
          corners.push_back(cv::Point3f(j * squareSize, i * squareSize, 0));
        }
      }
      break;
    case CalibrationSettings::CHARUCOBOARD:
      for (int i = 0; i < boardSize.height - 1; ++i) {
        for (int j = 0; j < boardSize.width - 1; ++j) {
          corners.push_back(cv::Point3f(j * squareSize, i * squareSize, 0));
        }
      }
      break;
    case CalibrationSettings::ASYMMETRIC_CIRCLES_GRID:
      for (int i = 0; i < boardSize.height; i++) {
        for (int j = 0; j < boardSize.width; j++) {
          corners.push_back(
              cv::Point3f((2 * j + i % 2) * squareSize, i * squareSize, 0));
        }
      }
      break;
    default:
      break;
  }
}
//! [board_corners]
static bool runCalibration(CalibrationSettings& s, cv::Size& imageSize,
                           cv::Mat& cameraMatrix,
                           cv::Mat& distCoeffs,
                           std::vector<std::vector<cv::Point2f>> imagePoints,
                           std::vector<cv::Mat>& rvecs, std::vector<cv::Mat>& tvecs,
                           std::vector<float>& reprojErrs, double& totalAvgErr,
                           std::vector<cv::Point3f>& newObjPoints, float grid_width,
                           bool release_object) {
  //! [fixed_aspect]
  cameraMatrix = cv::Mat::eye(3, 3, CV_64F);
  if (!s.useFisheye && s.flag & cv::CALIB_FIX_ASPECT_RATIO)
    cameraMatrix.at<double>(0, 0) = s.aspectRatio;
  //! [fixed_aspect]
  if (s.useFisheye) {
    distCoeffs = cv::Mat::zeros(4, 1, CV_64F);
  } else {
    distCoeffs = cv::Mat::zeros(8, 1, CV_64F);
  }

  std::vector<std::vector<cv::Point3f>> objectPoints(1);
  calcBoardCornerPositions(s.boardSize, s.calibrationSquareSize, objectPoints[0],
                           s.calibrationPattern);
  if (s.calibrationPattern == CalibrationSettings::Pattern::CHARUCOBOARD) {
    objectPoints[0][s.boardSize.width - 2].x =
        objectPoints[0][0].x + grid_width;
  } else {
    objectPoints[0][s.boardSize.width - 1].x =
        objectPoints[0][0].x + grid_width;
  }
  newObjPoints = objectPoints[0];

  objectPoints.resize(imagePoints.size(), objectPoints[0]);

  // Find intrinsic and extrinsic camera parameters
  double rms;

  if (s.useFisheye) {
    cv::Mat _rvecs, _tvecs;
    rms = cv::fisheye::calibrate(objectPoints, imagePoints, imageSize, cameraMatrix,
                             distCoeffs, _rvecs, _tvecs, s.flag);

    rvecs.reserve(_rvecs.rows);
    tvecs.reserve(_tvecs.rows);
    for (int i = 0; i < int(objectPoints.size()); i++) {
      rvecs.push_back(_rvecs.row(i));
      tvecs.push_back(_tvecs.row(i));
    }
  } else {
    int iFixedPoint = -1;
    if (release_object) iFixedPoint = s.boardSize.width - 1;
    rms = calibrateCameraRO(objectPoints, imagePoints, imageSize, iFixedPoint,
                            cameraMatrix, distCoeffs, rvecs, tvecs,
                            newObjPoints, s.flag | cv::CALIB_USE_LU);
  }

  if (release_object) {
    std::cout << "New board corners: " << std::endl;
    std::cout << newObjPoints[0] << std::endl;
    std::cout << newObjPoints[s.boardSize.width - 1] << std::endl;
    std::cout << newObjPoints[s.boardSize.width * (s.boardSize.height - 1)] << std::endl;
    std::cout << newObjPoints.back() << std::endl;
  }

  std::cout << "Re-projection error reported by calibrateCamera: " << rms << std::endl;

  bool ok = checkRange(cameraMatrix) && checkRange(distCoeffs);

  objectPoints.clear();
  objectPoints.resize(imagePoints.size(), newObjPoints);
  totalAvgErr = computeReprojectionErrors(objectPoints, imagePoints, rvecs,
                                          tvecs, cameraMatrix, distCoeffs,
                                          reprojErrs, s.useFisheye);

  return ok;
}

// Print camera parameters to the output file
static void saveCameraParams(CalibrationSettings& s, cv::Size& imageSize,
                             cv::Mat& cameraMatrix,
                             cv::Mat& distCoeffs, const std::vector<cv::Mat>& rvecs,
                             const std::vector<cv::Mat>& tvecs,
                             const std::vector<float>& reprojErrs,
                             const std::vector<std::vector<cv::Point2f>>& imagePoints,
                             double totalAvgErr,
                             const std::vector<cv::Point3f>& newObjPoints) {
  cv::FileStorage fs(s.outputFileName, cv::FileStorage::WRITE);

  time_t tm;
  time(&tm);
  struct tm* t2 = localtime(&tm);
  char buf[1024];
  strftime(buf, sizeof(buf), "%c", t2);

  fs << "calibration_time" << buf;

  if (!rvecs.empty() || !reprojErrs.empty())
    fs << "nr_of_frames" << (int)std::max(rvecs.size(), reprojErrs.size());
  fs << "image_width" << imageSize.width;
  fs << "image_height" << imageSize.height;
  fs << "board_width" << s.boardSize.width;
  fs << "board_height" << s.boardSize.height;
  fs << "square_size" << s.calibrationSquareSize;
  fs << "marker_size" << s.calibrationMarkerSize;

  if (!s.useFisheye && s.flag & cv::CALIB_FIX_ASPECT_RATIO)
    fs << "fix_aspect_ratio" << s.aspectRatio;

  if (s.flag) {
    std::stringstream flagsStringStream;
    if (s.useFisheye) {
      flagsStringStream << "flags:"
                        << (s.flag & cv::fisheye::CALIB_FIX_SKEW ? " +fix_skew"
                                                             : "")
                        << (s.flag & cv::fisheye::CALIB_FIX_K1 ? " +fix_k1" : "")
                        << (s.flag & cv::fisheye::CALIB_FIX_K2 ? " +fix_k2" : "")
                        << (s.flag & cv::fisheye::CALIB_FIX_K3 ? " +fix_k3" : "")
                        << (s.flag & cv::fisheye::CALIB_FIX_K4 ? " +fix_k4" : "")
                        << (s.flag & cv::fisheye::CALIB_RECOMPUTE_EXTRINSIC
                                ? " +recompute_extrinsic"
                                : "");
    } else {
      flagsStringStream
          << "flags:"
          << (s.flag & cv::CALIB_USE_INTRINSIC_GUESS ? " +use_intrinsic_guess" : "")
          << (s.flag & cv::CALIB_FIX_ASPECT_RATIO ? " +fix_aspectRatio" : "")
          << (s.flag & cv::CALIB_FIX_PRINCIPAL_POINT ? " +fix_principal_point" : "")
          << (s.flag & cv::CALIB_ZERO_TANGENT_DIST ? " +zero_tangent_dist" : "")
          << (s.flag & cv::CALIB_FIX_K1 ? " +fix_k1" : "")
          << (s.flag & cv::CALIB_FIX_K2 ? " +fix_k2" : "")
          << (s.flag & cv::CALIB_FIX_K3 ? " +fix_k3" : "")
          << (s.flag & cv::CALIB_FIX_K4 ? " +fix_k4" : "")
          << (s.flag & cv::CALIB_FIX_K5 ? " +fix_k5" : "");
    }
    fs.writeComment(flagsStringStream.str());
  }

  fs << "flags" << s.flag;

  fs << "fisheye_model" << s.useFisheye;

  fs << "camera_matrix" << cameraMatrix;
  fs << "distortion_coefficients" << distCoeffs;

  fs << "avg_reprojection_error" << totalAvgErr;
  if (s.writeExtrinsics && !reprojErrs.empty())
    fs << "per_view_reprojection_errors" << cv::Mat(reprojErrs);

  if (s.writeExtrinsics && !rvecs.empty() && !tvecs.empty()) {
    CV_Assert(rvecs[0].type() == tvecs[0].type());
    cv::Mat bigmat((int)rvecs.size(), 6, CV_MAKETYPE(rvecs[0].type(), 1));
    bool needReshapeR = rvecs[0].depth() != 1 ? true : false;
    bool needReshapeT = tvecs[0].depth() != 1 ? true : false;

    for (size_t i = 0; i < rvecs.size(); i++) {
      cv::Mat r = bigmat(cv::Range(int(i), int(i + 1)), cv::Range(0, 3));
      cv::Mat t = bigmat(cv::Range(int(i), int(i + 1)), cv::Range(3, 6));

      if (needReshapeR)
        rvecs[i].reshape(1, 1).copyTo(r);
      else {
        //*.t() is MatExpr (not Mat) so we can use assignment operator
        CV_Assert(rvecs[i].rows == 3 && rvecs[i].cols == 1);
        r = rvecs[i].t();
      }

      if (needReshapeT)
        tvecs[i].reshape(1, 1).copyTo(t);
      else {
        CV_Assert(tvecs[i].rows == 3 && tvecs[i].cols == 1);
        t = tvecs[i].t();
      }
    }
    fs.writeComment(
        "a set of 6-tuples (rotation vector + translation vector) for each "
        "view");
    fs << "extrinsic_parameters" << bigmat;
  }

  if (s.writePoints && !imagePoints.empty()) {
    cv::Mat imagePtMat((int)imagePoints.size(), (int)imagePoints[0].size(),
                   CV_32FC2);
    for (size_t i = 0; i < imagePoints.size(); i++) {
      cv::Mat r = imagePtMat.row(int(i)).reshape(2, imagePtMat.cols);
      cv::Mat imgpti(imagePoints[i]);
      imgpti.copyTo(r);
    }
    fs << "image_points" << imagePtMat;
  }

  if (s.writeGrid && !newObjPoints.empty()) {
    fs << "grid_points" << newObjPoints;
  }
}

//! [run_and_save]
bool runCalibrationAndSave(CalibrationSettings& s, cv::Size imageSize,
                           cv::Mat& cameraMatrix,
                           cv::Mat& distCoeffs,
                           std::vector<std::vector<cv::Point2f> > imagePoints,
                           float grid_width, bool release_object) {
  std::vector<cv::Mat> rvecs, tvecs;
  std::vector<float> reprojErrs;
  double totalAvgErr = 0;
  std::vector<cv::Point3f> newObjPoints;

  bool ok = runCalibration(s, imageSize, cameraMatrix, distCoeffs, imagePoints,
                           rvecs, tvecs, reprojErrs, totalAvgErr, newObjPoints,
                           grid_width, release_object);
  std::cout << (ok ? "Calibration succeeded" : "Calibration failed")
       << ". avg re projection error = " << totalAvgErr << std::endl;

  if (ok)
    saveCameraParams(s, imageSize, cameraMatrix, distCoeffs, rvecs, tvecs,
                     reprojErrs, imagePoints, totalAvgErr, newObjPoints);
  return ok;
}
//! [run_and_save]

}  // namespace

std::optional<cv::aruco::Dictionary> CreateArucoDict(CalibrationSettings &s) {
  cv::aruco::Dictionary dictionary;
  if (s.arucoDictFileName == "") {
    cv::aruco::PredefinedDictionaryType arucoDict;
    if (s.arucoDictName == "DICT_4X4_50") {
      arucoDict = cv::aruco::DICT_4X4_50;
    } else if (s.arucoDictName == "DICT_4X4_100") {
      arucoDict = cv::aruco::DICT_4X4_100;
    } else if (s.arucoDictName == "DICT_4X4_250") {
      arucoDict = cv::aruco::DICT_4X4_250;
    } else if (s.arucoDictName == "DICT_4X4_1000") {
      arucoDict = cv::aruco::DICT_4X4_1000;
    } else if (s.arucoDictName == "DICT_5X5_50") {
      arucoDict = cv::aruco::DICT_5X5_50;
    } else if (s.arucoDictName == "DICT_5X5_100") {
      arucoDict = cv::aruco::DICT_5X5_100;
    } else if (s.arucoDictName == "DICT_5X5_250") {
      arucoDict = cv::aruco::DICT_5X5_250;
    } else if (s.arucoDictName == "DICT_5X5_1000") {
      arucoDict = cv::aruco::DICT_5X5_1000;
    } else if (s.arucoDictName == "DICT_6X6_50") {
      arucoDict = cv::aruco::DICT_6X6_50;
    } else if (s.arucoDictName == "DICT_6X6_100") {
      arucoDict = cv::aruco::DICT_6X6_100;
    } else if (s.arucoDictName == "DICT_6X6_250") {
      arucoDict = cv::aruco::DICT_6X6_250;
    } else if (s.arucoDictName == "DICT_6X6_1000") {
      arucoDict = cv::aruco::DICT_6X6_1000;
    } else if (s.arucoDictName == "DICT_7X7_50") {
      arucoDict = cv::aruco::DICT_7X7_50;
    } else if (s.arucoDictName == "DICT_7X7_100") {
      arucoDict = cv::aruco::DICT_7X7_100;
    } else if (s.arucoDictName == "DICT_7X7_250") {
      arucoDict = cv::aruco::DICT_7X7_250;
    } else if (s.arucoDictName == "DICT_7X7_1000") {
      arucoDict = cv::aruco::DICT_7X7_1000;
    } else if (s.arucoDictName == "DICT_ARUCO_ORIGINAL") {
      arucoDict = cv::aruco::DICT_ARUCO_ORIGINAL;
    } else if (s.arucoDictName == "DICT_APRILTAG_16h5") {
      arucoDict = cv::aruco::DICT_APRILTAG_16h5;
    } else if (s.arucoDictName == "DICT_APRILTAG_25h9") {
      arucoDict = cv::aruco::DICT_APRILTAG_25h9;
    } else if (s.arucoDictName == "DICT_APRILTAG_36h10") {
      arucoDict = cv::aruco::DICT_APRILTAG_36h10;
    } else if (s.arucoDictName == "DICT_APRILTAG_36h11") {
      arucoDict = cv::aruco::DICT_APRILTAG_36h11;
    } else {
      std::cout << "incorrect name of aruco dictionary \n";
      return std::nullopt;
    }

    dictionary = cv::aruco::getPredefinedDictionary(arucoDict);
  } else {
    cv::FileStorage dict_file(s.arucoDictFileName, cv::FileStorage::Mode::READ);
    cv::FileNode fn(dict_file.root());
    dictionary.readDictionary(fn);
  }
  return dictionary;
}

std::optional<CameraParameters> CalibrateAndSaveCameraParameters(CalibrationSettings &s) {
  
  //! [file_read]

  if (!s.goodInput) {
    std::cout << "Invalid input detected. " << std::endl;
    return std::nullopt;
  }

  float grid_width = s.calibrationSquareSize * (s.boardSize.width - 1);
  if (s.calibrationPattern == CalibrationSettings::Pattern::CHARUCOBOARD) {
    grid_width = s.calibrationSquareSize * (s.boardSize.width - 2);
  }

  bool release_object = false;

  // create CharucoBoard
  cv::aruco::Dictionary dictionary;
  if (s.calibrationPattern == CalibrationSettings::CHARUCOBOARD) {
    std::optional<cv::aruco::Dictionary> optional_dict = CreateArucoDict(s);
    if (!optional_dict.has_value()) {
      return std::nullopt;
    }
    dictionary = optional_dict.value();
  } else {
    // default dictionary
    dictionary = cv::aruco::getPredefinedDictionary(0);
  }
  cv::aruco::CharucoBoard ch_board({s.boardSize.width, s.boardSize.height},
                                   s.calibrationSquareSize, s.calibrationMarkerSize, dictionary);
  cv::aruco::CharucoDetector ch_detector(ch_board);
  std::vector<int> markerIds;

  std::vector<std::vector<cv::Point2f>> imagePoints;
  cv::Mat cameraMatrix, distCoeffs;
  cv::Size imageSize;
  int mode =
      s.inputType == CalibrationSettings::IMAGE_LIST ? CAPTURING : DETECTION;
  clock_t prevTimestamp = 0;
  const cv::Scalar RED(0, 0, 255), GREEN(0, 255, 0);
  const char ESC_KEY = 27;
  //! [get_input]

  for (;;) {
    cv::Mat view;
    bool blinkOutput = false;

    view = s.nextImage();

    //-----  If no more image, or got enough, then stop calibration and show
    //result -------------
    if (mode == CAPTURING && imagePoints.size() >= (size_t)s.nrFrames) {
      if (runCalibrationAndSave(s, imageSize, cameraMatrix, distCoeffs,
                                imagePoints, grid_width, release_object))
        mode = CALIBRATED;
      else
        mode = DETECTION;
    }
    if (view.empty())  // If there are no more images stop the loop
    {
      // if calibration threshold was not reached yet, calibrate now
      if (mode != CALIBRATED && !imagePoints.empty())
        runCalibrationAndSave(s, imageSize, cameraMatrix, distCoeffs,
                              imagePoints, grid_width, release_object);
      break;
    }
    //! [get_input]

    imageSize = view.size();  // Format input image.
    if (s.flipVertical) flip(view, view, 0);

    //! [find_pattern]
    std::vector<cv::Point2f> pointBuf;

    bool found;

    int chessBoardFlags = cv::CALIB_CB_ADAPTIVE_THRESH | cv::CALIB_CB_NORMALIZE_IMAGE;

    if (!s.useFisheye) {
      // fast check erroneously fails with high distortions like fisheye
      chessBoardFlags |= cv::CALIB_CB_FAST_CHECK;
    }

    switch (s.calibrationPattern)  // Find feature points on the input format
    {
      case CalibrationSettings::CHESSBOARD:
        found =
            findChessboardCorners(view, s.boardSize, pointBuf, chessBoardFlags);
        break;
      case CalibrationSettings::CHARUCOBOARD:
        ch_detector.detectBoard(view, pointBuf, markerIds);
        found = pointBuf.size() ==
                (size_t)((s.boardSize.height - 1) * (s.boardSize.width - 1));
        break;
      case CalibrationSettings::CIRCLES_GRID:
        found = findCirclesGrid(view, s.boardSize, pointBuf);
        break;
      case CalibrationSettings::ASYMMETRIC_CIRCLES_GRID:
        found = findCirclesGrid(view, s.boardSize, pointBuf,
                                cv::CALIB_CB_ASYMMETRIC_GRID);
        break;
      default:
        found = false;
        break;
    }
    
    //! [find_pattern]

    //! [pattern_found]
    if (found)  // If done with success,
    {
      // improve the found corners' coordinate accuracy for chessboard
      if (s.calibrationPattern == CalibrationSettings::CHESSBOARD) {
        cv::Mat viewGray;
        cvtColor(view, viewGray, cv::COLOR_BGR2GRAY);
        cornerSubPix(
            viewGray, pointBuf, cv::Size(s.windowSize, s.windowSize), cv::Size(-1, -1),
            cv::TermCriteria(cv::TermCriteria::EPS + cv::TermCriteria::COUNT, 30, 0.0001));
      }

      if (mode ==
              CAPTURING &&  // For camera only take new samples after delay time
          (!s.inputCapture.isOpened() ||
           clock() - prevTimestamp > s.delay * 1e-3 * CLOCKS_PER_SEC)) {
        imagePoints.push_back(pointBuf);
        prevTimestamp = clock();
        blinkOutput = s.inputCapture.isOpened();
      }

      // Draw the corners.
      if (s.calibrationPattern == CalibrationSettings::CHARUCOBOARD)
        drawChessboardCorners(
            view, cv::Size(s.boardSize.width - 1, s.boardSize.height - 1),
            cv::Mat(pointBuf), found);
      else
        drawChessboardCorners(view, s.boardSize, cv::Mat(pointBuf), found);
    }
    //! [pattern_found]
    //----------------------------- Output Text
    //------------------------------------------------
    //! [output_text]
    std::string msg = (mode == CAPTURING)  ? "100/100"
                 : mode == CALIBRATED ? "Calibrated"
                                      : "Press 'g' to start";
    int baseLine = 0;
    cv::Size textSize = cv::getTextSize(msg, 1, 1, 1, &baseLine);
    cv::Point textOrigin(view.cols - 2 * textSize.width - 10,
                     view.rows - 2 * baseLine - 10);

    if (mode == CAPTURING) {
      if (s.showUndistorted)
        msg = cv::format("%d/%d Undist", (int)imagePoints.size(), s.nrFrames);
      else
        msg = cv::format("%d/%d", (int)imagePoints.size(), s.nrFrames);
    }

    putText(view, msg, textOrigin, 1, 1, mode == CALIBRATED ? GREEN : RED);

    if (blinkOutput) bitwise_not(view, view);
    //! [output_text]
    //------------------------- Video capture  output  undistorted
    //------------------------------
    //! [output_undistorted]
    if (mode == CALIBRATED && s.showUndistorted) {
      cv::Mat temp = view.clone();
      if (s.useFisheye) {
        cv::Mat newCamMat;
        cv::fisheye::estimateNewCameraMatrixForUndistortRectify(
            cameraMatrix, distCoeffs, imageSize, cv::Matx33d::eye(), newCamMat, 1);
        cv::fisheye::undistortImage(temp, view, cameraMatrix, distCoeffs,
                                    newCamMat);
      } else
        undistort(temp, view, cameraMatrix, distCoeffs);
    }
    //! [output_undistorted]
    //------------------------------ Show image and check for input commands
    //-------------------
    //! [await_input]
    imshow("Image View", view);
    char key = (char)cv::waitKey(s.inputCapture.isOpened() ? 50 : s.delay);

    if (key == ESC_KEY) break;

    if (key == 'u' && mode == CALIBRATED)
      s.showUndistorted = !s.showUndistorted;

    if (s.inputCapture.isOpened() && key == 'g') {
      mode = CAPTURING;
      imagePoints.clear();
    }
    //! [await_input]
  }
  
  CameraParameters parameters;
  parameters.insintric_camera_parms = cameraMatrix;
  parameters.distortion_mat = distCoeffs;

  return parameters;
}

std::optional<CameraParameters> GetCameraParametersFromFile(
    CalibrationSettings &camera_settings) {
  //! [file_read]
  cv::FileStorage fs(camera_settings.outputFileName,
                     cv::FileStorage::READ);  // Read the settings
  if (!fs.isOpened()) {
    return std::nullopt;
  }
  CameraParameters params;
  try {
    fs["distortion_coefficients"] >> params.distortion_mat;
    fs["camera_matrix"] >> params.insintric_camera_parms;
  } catch (...) {
    fs.release();
    return std::nullopt;
  }
  return params;
}

std::optional<const CameraParameters> CalulateCameraParameters(
    CalibrationSettings& camera_settings) {
  std::optional<CameraParameters> cachedCameraParameters =
      GetCameraParametersFromFile(camera_settings);
  if (cachedCameraParameters.has_value()) {
    return cachedCameraParameters;
  }
  return CalibrateAndSaveCameraParameters(camera_settings);
}
}  // namespace CameraMarkerServer