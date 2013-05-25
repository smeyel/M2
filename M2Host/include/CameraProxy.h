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
	long long lastImageTakenTimestamp;

	CameraProxy();
	CameraProxy(PhoneProxy *aPhoneProxy, Camera *aCamera);
	~CameraProxy();

	// Wrappers for PhoneProxy
	void Connect(const char *ip, int port);
	void Disconnect();

	// CaptureImage
	void CaptureImage(long long desiredTimestamp=0); // calls CaptureImage(lastImageTaken)
	void CaptureImage(long long desiredTimestamp, Mat *target);	// Does not use lastImageTaken

	bool TryCalibration(bool showResultOnImage=false);	// calls TryCalibration(lastImageTaken,showResultOnImage)
	bool TryCalibration(Mat *image, bool showResultOnImage=false);	// does not use lastImageTaken

	// Capture image and try to find chessboard
	bool CaptureAndTryCalibration(bool showResultOnImage=false);

	// Ask for images until calibration is successful
	bool CaptureUntilCalibrated(int maxFrameNum);

	// Camera wrapper: return Ray3D for given image coordinates
	Ray pointImg2World(Point2f pImg);

	// Image transfer speed test
	// Use only alone (no prior or later operations) to keep the statistics clean
	// Use Connect before and Disconnect after calling it.
	void PerformCaptureSpeedMeasurement_A(int frameNumber=100, const char *resultfilename = NULL);
};

#endif
