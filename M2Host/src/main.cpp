#include <iostream>
//#include <sstream>
//#include <time.h>
#include <fstream>	// For marker data export into file...

//#include <windows.h>	// for sleep

#include <opencv2/opencv.hpp>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>

#include "../include/PhoneProxy.h"

#include "JpegMessage.h"
#include "MatImageMessage.h"
#include "MeasurementLogMessage.h"

#include "myconfigmanager.h"

#include "chessboarddetector.h"
#include "camera.h"


//#include "VideoInputFactory.h"
//#include "VideoInputPs3EyeParameters.h"

#include "TimeMeasurement.h"
//#include "Logger.h"
#include "FileLogger.h"
#include "CameraProxy.h"

#include "../include/TimeMeasurementCodeDefines.h"

using namespace cv;
using namespace std;

MyConfigManager configManager;
//LogConfigTime::TimeMeasurement timeMeasurement;
int frameIdx = 0;
char *configfilename = "m2_default.ini";	// 1st command line parameter overrides it (if exists)

const char *imageWindowName = "Received JPEG";

void M2_TrackingTest(CameraProxy *camProxy)
{
	enum _mode
	{
		nop,
		finished,
		chessboard,
		tracking
	} mode = nop;

	while(mode != finished)
	{
		camProxy->CaptureImage();
		// Image processing
		OPENCV_ASSERT(camProxy->lastImageTaken->type()==CV_8UC3,"M2_Tracking","Mat type not CV_8UC3.");
		switch (mode)
		{
		case chessboard:
			// Detect chessboard
			camProxy->TryCalibration(true);
			putText( *(camProxy->lastImageTaken), string("CHESSBOARD MODE"), cvPoint( camProxy->lastImageTaken->cols-200, 20 ), FONT_HERSHEY_DUPLEX, 0.5, CV_RGB(255,255,255) );

			break;
		case tracking:
			// TODO: Detect marker
			putText( *(camProxy->lastImageTaken), string("TRACKING MODE"), cvPoint( camProxy->lastImageTaken->cols-200, 20 ), FONT_HERSHEY_DUPLEX, 0.5, CV_RGB(255,255,255) );

			break;
		}

		// Showing the picture results
		OPENCV_ASSERT(camProxy->lastImageTaken->type()==CV_8UC3,"M2_Tracking","Mat type not CV_8UC3. Was it received successfully?");
		imshow(imageWindowName,*(camProxy->lastImageTaken));

		// If the images are shown, have to wait to allow it to display...
		// Process possible keypress
		char ch = waitKey(50);
		switch (ch)
		{
		case -1:	// Nothing pressed
			break;
		case 27:	// Escape
		case 'x':
			mode = finished;
			cout << "Mode: finished" << endl;
			break;
		case 'n':
			mode = nop;
			cout << "Mode: NOP" << endl;
			break;
		case 'c':
			mode = chessboard;
			cout << "Mode: chessboard" << endl;
			break;
		case 't':
			mode = tracking;
			cout << "Mode: tracking" << endl;
			break;
		default:
			cout << "Keys:" << endl
				<< "Esc	quit" << endl
				<< "x	quit" << endl
				<< "n	no operation" << endl
				<< "c	chessboard detection" << endl
				<< "t	tracking the marker" << endl;
		}

		frameIdx++;
	}

	cout << "Necessary pictures taken. Receiving measurement log..." << endl;
	camProxy->phoneproxy->RequestLog();

	JsonMessage *msg = NULL;
	MeasurementLogMessage *logMsg = NULL;
	Mat img;
	msg = camProxy->phoneproxy->ReceiveNew();
	if (msg->getMessageType() == MeasurementLog)
	{
		logMsg = (MeasurementLogMessage *)msg;
		logMsg->writeAuxFile((char*)(configManager.remoteMLogFilename.c_str()));
	}
	else
	{
		cout << "Error... received something else than JPEG... see the log for details!" << endl;
		Logger::getInstance()->Log(Logger::LOGLEVEL_ERROR,"M2Host","Received something else than JPEG image:\n");
		msg->log();
	}
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

	//M2_SpeedTest(camProxy);
	//M2_TrackingTest(camProxy);
	camProxy->PerformCaptureSpeedMeasurement_A(100,configManager.MLogFilename.c_str());

	cout << "Main task finished" << endl;
	// --------------------------- Closing...
	cout << "Disconnecting..." << endl;
	camProxy->Disconnect();

	delete camProxy;
	cout << "Done." << endl;
}
