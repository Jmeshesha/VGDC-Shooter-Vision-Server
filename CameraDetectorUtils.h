#ifndef CAMERA_DETECTOR_UTILS_H_
#define CAMERA_DETECTOR_UTILS_H_
#include <opencv2/core.hpp>
#include <opencv2/core/types.hpp>
#include <opencv2/videoio.hpp>

namespace CameraMarkerServer {
	struct Pose {
		float translate_x;
		float translate_y;
		float translate_z;
		float rotation_x;
		float rotation_y;
		float rotation_z;
	};
	struct CalibrationParameters {
          cv::Mat insintric_camera_parms;
          cv::Mat distortion_mat;
	};

	bool DetectPose(Pose &output_pose, const cv::Mat camera_frame, const CalibrationParameters calibration_params);

	bool CalibrateCamera(CalibrationParameters &output_calibration_params,
                             const std::string images_path, const std::string calibration_settings_path);
	
}  // namespace CameraMarkerServer
#endif     // CAMERA_DETECTOR_UTILS_H_CAMERA_DETECTOR_UTILS_H_