#include <iostream>
//#include <sstream>
//#include <time.h>
#include <fstream>	// For marker data export into file...

//#include <windows.h>	// for sleep

#include <opencv2/opencv.hpp>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>

// Communication
#include "../include/PhoneProxy.h"
#include "JpegMessage.h"
#include "MatImageMessage.h"
#include "MeasurementLogMessage.h"
#include "CameraProxy.h"

// Configuration, log, time measurement
#include "myconfigmanager.h"
#include "TimeMeasurement.h"
#include "../include/TimeMeasurementCodeDefines.h"
#include "FileLogger.h"

// SMEyeL image processing and 3D world headers
//#include "chessboarddetector.h"
//#include "camera.h"

// Marker detection
//#include "DetectionResultExporterBase.h"
//#include "MarkerBase.h"
//#include "MarkerCC2Tracker.h"

using namespace cv;
using namespace std;

MyConfigManager configManager;
int frameIdx = 0;
char *configfilename = "m2_default.ini";	// 1st command line parameter overrides it (if exists)


/** TimeSyncBeacon time synchronization test
	- Asks the camera to take a picture, retrieves image and timestamp
	- Reads the TymeSyncBeacon time (Beacon Time) from the image
	- repeat many times:
		- Chooses a DesiredBeaconTime not too far away
		- Calculates the DesiredTimeStamp corresponding to the Beacon Time
		- Asks the camera to take a picture in the given DesiredTimeStamp
		- Retrieves the image and its timestamp
		- Calculates timing error and writes it to the log

	Calculated results:
	- "ImageBeaconTime - DesiredBeaconTime"
		- difference
		- its standard deviation
		- dependency from the camera settings (various exposure times)
*/
void M2_TimeSyncTest(CameraProxy *camProxy, int captureNum)
{
	// Open measurement results file
	std::ofstream mlog;
	mlog.open(configManager.remoteMLogFilename.c_str(),std::ofstream::binary);

	// Take initial picture as soon as possible
	camProxy->CaptureImage(0);
	Mat *image = camProxy->lastImageTaken;
	long long startTimeStampUs = camProxy->lastImageTakenTimestamp;
	long long startBeaconTimeMs = 0;//readTimeStampBeaconMsecFromImage(camProxy->lastImageTaken);	// Returns in msec

	mlog << "M2_TimeSyncTest measurement result" << endl
		 << "desiredBeaconTimeMs;lastBeaconTimeMs;desiredTimeStampUs;lastTimeStampUs" << endl;
	cout << "desiredBeaconTimeMs - lastBeaconTimeMs" << endl;
	long long lastBeaconTimeMs = startBeaconTimeMs;	// Beacon time of the last image
	while (frameIdx < captureNum)
	{
		// Choose DesiredBeaconTime (+2 seconds)
		long long desiredBeaconTimeMs = lastBeaconTimeMs + 2000;
		// Calculate DesiredTimeStamp
		long long desiredTimeStampUs = ((desiredBeaconTimeMs - startBeaconTimeMs) * 1000) + startTimeStampUs;
		// Take picture at desiredTimeStamp
		camProxy->CaptureImage(desiredTimeStampUs);
		// Save image timestamp
		long long lastTimeStampUs = camProxy->lastImageTakenTimestamp;
		// Read image BeaconTime
		long long lastBeaconTimeMs = 0;//readTimeStampBeaconMsecFromImage(camProxy->lastImageTaken);

		mlog << desiredBeaconTimeMs << ";" << lastBeaconTimeMs << ";" << desiredTimeStampUs << ";" << lastTimeStampUs << endl;
		cout << "desired-last BeaconTime: " << (desiredBeaconTimeMs - lastBeaconTimeMs) << " ms" << endl;

		frameIdx++;
	}

	mlog.flush();
	mlog.close();
}

/** Implementation of M2 scenario
*/
int main(int argc, char *argv[])
{
	if (argc>=2)
	{
		// INI file is given as command line parameter
		configfilename = argv[1];
	}
	// Setup config management
	configManager.init(configfilename);

	// Setup statistics output file
	Logger *logger = new FileLogger(configManager.logFileName.c_str());
	logger->SetLogLevel(Logger::LOGLEVEL_INFO);
	Logger::getInstance()->Log(Logger::LOGLEVEL_VERBOSE,"M2Host","M2Host started\n");

	cout << "Log is written to: " << configManager.logFileName << endl;

	// Write current time and date to log
    time_t t = time(0);   // get time now
    struct tm * now = localtime( & t );
	Logger::getInstance()->Log(Logger::LOGLEVEL_VERBOSE,"M2Host","Current time: %d-%d-%d, %d:%d:%d\n",
		(now->tm_year + 1900),(now->tm_mon + 1),now->tm_mday,now->tm_hour,now->tm_min,now->tm_sec );

	Logger::getInstance()->Log(Logger::LOGLEVEL_VERBOSE,"M2Host","Configuration: %s\n",configfilename);
	Logger::getInstance()->Log(Logger::LOGLEVEL_VERBOSE,"M2Host","Local measurement log: %s\nRemote measurement log: %s\n",
		configManager.localMLogFilename.c_str(), configManager.remoteMLogFilename.c_str());

	// Setup time management
	// Waiting a little to allow other processes to init...
	// Useful if started together with CamClient which needs to start its server.
	cout << "Waiting 3s..." << endl;
#ifdef WIN32
	Sleep(3000);
#else
#error TODO: Sleep not implemented for non-Win32.
#endif

	CameraProxy *camProxy = new CameraProxy();
	// Prepare camera and detector objects
	//ChessboardDetector detector(Size(9,6),36.1);	// Chessboard cell size is 36x36mm, using CameraProxy default
	camProxy->camera->cameraID=0;
	camProxy->camera->isStationary=false;
	camProxy->camera->loadCalibrationData(configManager.camIntrinsicParamsFileName.data());

	cout << "Connecting..." << endl;
	camProxy->Connect(configManager.phoneIpAddress.c_str(),configManager.phonePort);

	// --------------------------- Execute main task
	cout << "Main task started" << endl;

	// ---Currently, multiple possible measurements are supported.
	// - Original "capture 100 frames and report capture times" measurement
	camProxy->PerformCaptureSpeedMeasurement_A(100,configManager.MLogFilename.c_str());
	// - Reading TimeSyncBeacon measurement
	//M2_TimeSyncTest(camProxy,10);

	cout << "Main task finished" << endl;
	// --------------------------- Closing...
	cout << "Disconnecting..." << endl;
	camProxy->Disconnect();

	delete camProxy;
	cout << "Done." << endl;
}
