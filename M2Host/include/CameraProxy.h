#ifndef __CAMERAPROXY_H
#define __CAMERAPROXY_H

#include "camera.h"
#include "PhoneProxy.h"
#include "chessboarddetector.h"

/** Wraps a PhoneProxy, a Camera and a ChessboardDetector.
	Provides a convenient interface for basic operations with remote cameras.
	Exposes internal (wrapped) objects for full functionality access.

	To initialize:
	- Instantiate CameraProxy
	- You may want to set chessboarddetector.chessboard (but default 9x6 board with 36mm grid size is already set)
	- You may want to set camera->cameraID, camera->isStationary and call camera->loadCalibrationData()
	- Call Connect()

	Usage: beside the many other functions, most important ones are the following:
	- CaptureImage()
	- TryCalibration()
	- CaptureAndTryCalibration() wraps the pervious two
	- CaptureUntilCalibrated()

	Shutting down:
	- Call Disconnect()

	To measure response time of remote cameras, the PerformCaptureSpeedMeasurement_A() function is an all-in-one
	speed measurement method. Call this after proper initialization and disconnect after that.
	Other functions operations should not be performed with this measurement to clean timing statistics clean.
*/
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
	/** Pointer to the used PhoneProxy. Initialized with a default one. */
	PhoneProxy *phoneproxy;
	/** Pointer to the used Camera. Initialized with a default one. */
	Camera *camera;
	/** Pointer to the used ChessboardDetector. Initialized with a default one. */
	ChessboardDetector *chessboarddetector;

	/** Pointer to the last captured image.
		@warning Do not access this while a new frame is received! Not thread safe!
	*/
	Mat *lastImageTaken;	// Not thread safe!
	/** The timestamp of the last received image.
		@warning Do not access this while a new frame is received! Not thread safe!
	*/
	long long lastImageTakenTimestamp;

	/** Constrcutor setting default PhoneProxy, Camera and ChessboardDetector */
	CameraProxy();
	/** Constrcutor setting custom PhoneProxy and Camera, using default ChessboardDetector
		@param aPhoneProxy	PhoneProxy to be used
		@param aCamera		Camera to be used
	*/
	CameraProxy(PhoneProxy *aPhoneProxy, Camera *aCamera);
	/** Destructor */
	~CameraProxy();

	/** Wrapper for PhoneProxy->Connect() for convenience
		@param	ip		IPv4 address of the remote camera, as a string like "127.0.0.1"
		@param	port	TCP port the remote camera is listening to
	*/
	void Connect(const char *ip, int port);
	/** Wrapper for PhoneProxy->Disconnect() for convenience */
	void Disconnect();

	// CaptureImage
	/** Captures an image. Result can be accessed via lastImageTaken 
		@param desiredTimestamp	The desired timestamp of the image
	*/
	void CaptureImage(long long desiredTimestamp=0); // calls CaptureImage(lastImageTaken)
	/** Captures an image. Does not modify the image lastImageTaken.
		If target==lastImageTaken, it sets lastImageTakenTimestamp. Otherwise the received
		timestamp cannot be accessed.
		@param desiredTimestamp	The desired timestamp of the image
		@param target			The Mat the captured image is stored in.
	*/
	void CaptureImage(long long desiredTimestamp, Mat *target);	// Does not use lastImageTaken

	/** Try to find a chessboard on the last taken image (accessible via lastImageTaken)
		@param showResultOnImage	If true, calibration data is written on the image.
		@returns	Returns true is calibration was successful (chassboard points were found).
	*/
	bool TryCalibration(bool showResultOnImage=false);	// calls TryCalibration(lastImageTaken,showResultOnImage)
	/** Try to find a chessboard on the given image
		@param image				The image where the chessboard is looked for.
		@param showResultOnImage	If true, calibration data is written on the image.
		@returns	Returns true is calibration was successful (chassboard points were found).
	*/
	bool TryCalibration(Mat *image, bool showResultOnImage=false);	// does not use lastImageTaken

	/** Capture image and try to find chessboard
		@param	showResultOnImage	If true, result is shown on the image.
		@returns	Returns true if the calibration was successful.
	*/
	bool CaptureAndTryCalibration(bool showResultOnImage=false);

	/** Captures images until calibration is successful.
		@param maxFrameNum	Maximal number of frames to try with.
		@returns	Returns true if calibration was successful.
	*/
	bool CaptureUntilCalibrated(int maxFrameNum);

	/** Return Ray3D for given image coordinates.
		Wrapper for Camera->pointImg2World().
		@param pImg	Point in image coordinates
		@returns	Returns the 3D ray corresponding pImg.
		@warning The camera has to be calibrated before using this function!
	*/
	Ray pointImg2World(Point2f pImg);

	/** Image transfer speed measurement
		Use only alone (no prior or later operations) to keep the statistics clean
		Use Connect before and Disconnect after calling it.
		@param frameNumber	The number of frames to capture after each other as fast as possible.
		@param resultfilename	The name of the file the resuls are written to.
	*/
	void PerformCaptureSpeedMeasurement_A(int frameNumber=100, const char *resultfilename = NULL);
};

#endif
