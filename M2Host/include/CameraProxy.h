#ifndef __CAMERAPROXY_H
#define __CAMERAPROXY_H

#include "camera.h"
#include "PhoneProxy.h"
#include "chessboarddetector.h"


class CameraProxy
{
private:
	// Defaults set by constructor, may be overridden
	PhoneProxy *default_phoneproxy;
	Camera *default_camera;	// Do not forget: cameraID, isStationary, loadCalibrationData
	ChessboardDetector *default_chessboarddetector;	// May overwrite its chessboard...

	// used by constructors
	void initDefaults();

public:
	PhoneProxy *phoneproxy;
	Camera *camera;
	ChessboardDetector *chessboarddetector;
	Mat *lastImageTaken;	// Not thread safe!

	CameraProxy();
	CameraProxy(PhoneProxy *aPhoneProxy, Camera *aCamera);
	~CameraProxy();

	// CaptureImage
	void CaptureImage(); // calls CaptureImage(lastImageTaken)
	void CaptureImage(Mat *target);	// Does not use lastImageTaken

	bool TryCalibration(bool showResultOnImage=false);	// calls TryCalibration(lastImageTaken,showResultOnImage)
	bool TryCalibration(Mat *image, bool showResultOnImage=false);	// does not use lastImageTaken

	// Capture image and try to find chessboard
	bool CaptureAndTryCalibration(bool showResultOnImage=false);

	// Ask for images until calibration is successful
	bool CaptureUntilCalibrated(int maxFrameNum);

	// Return Ray3D for given image coordinates

};

#endif
