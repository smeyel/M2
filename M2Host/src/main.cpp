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

//#include "VideoInputFactory.h"
//#include "VideoInputPs3EyeParameters.h"

#include "TimeMeasurement.h"
//#include "Logger.h"
#include "FileLogger.h"

#include "../include/TimeMeasurementCodeDefines.h"

using namespace cv;
using namespace std;

MyConfigManager configManager;
LogConfigTime::TimeMeasurement timeMeasurement;
int frameIdx = 0;
char *configfilename = "m2_default.ini";	// 1st command line parameter overrides it (if exists)

const char *imageWindowName = "Received JPEG";

void M2_SpeedTest(PhoneProxy proxy)
{
	bool running=true;

	while(running)
	{
		// Request image from the phone

		char tmpBuff[100];

		_int64 desiredTimeStamp = 0;
		_int64 last1PictureTimeStamp = 0;	// Last timestamp
		_int64 last2PictureTimeStamp = 0;	// Timestamp before the last one
		_int64 interPictureTime = 0;

		for(int i=0; i<100; i++)	// First 2 photos do not have desired timestamp...
		{
			frameIdx++;
			// Calculate desiredTimeStamp
			if (interPictureTime==0 && last2PictureTimeStamp>0)
			{
				// Calculate time between the first two images
				interPictureTime = last1PictureTimeStamp - last2PictureTimeStamp;
			}

			if (interPictureTime==0 || !configManager.useDesiredTimestamp)
			{
				// Unknown inter-picture time (we do not know the frequency of the timestamp!!!)
				//	(...or usage of this function is disabled from configuration.)
				desiredTimeStamp = 0;
			}
			else
			{
				desiredTimeStamp = last1PictureTimeStamp + interPictureTime * 2;
			}

			// Asking for a picture
			timeMeasurement.start(M2::TimeMeasurementCodeDefs::FrameAll);
			cout << "Capture No " << i << "..." << endl;
			timeMeasurement.start(M2::TimeMeasurementCodeDefs::Send);
			proxy.RequestPhoto(desiredTimeStamp);
			timeMeasurement.finish(M2::TimeMeasurementCodeDefs::Send);

			// Receiving picture
			timeMeasurement.start(M2::TimeMeasurementCodeDefs::WaitAndReceive);

			JsonMessage *msg = NULL;
			Mat img;
			bool isImgValid = false;
			msg = proxy.ReceiveNew();
			if (msg->getMessageType() == Jpeg)
			{
				JpegMessage *jpegMsg = NULL;
				jpegMsg = (JpegMessage *)msg;
				jpegMsg->Decode(&img);
				isImgValid = true;
			}
			else if (msg->getMessageType() == MatImage)
			{
				MatImageMessage *matimgMsg = NULL;
				matimgMsg = (MatImageMessage *)msg;
				if (matimgMsg->size != 0)
				{
					matimgMsg->Decode();
					matimgMsg->getMat()->copyTo(img);	// TODO: avoid this copy...
					isImgValid = true;
				}
			}
			else
			{
				cout << "Error... received something else than JPEG... see the log for details!" << endl;
				Logger::getInstance()->Log(Logger::LOGLEVEL_ERROR,"M2Host","Received something else than JPEG image:\n");
				msg->log();
			}
			delete msg;
			msg = NULL;
			timeMeasurement.finish(M2::TimeMeasurementCodeDefs::WaitAndReceive);

			// Showing the picture
			if (configManager.showImage)
			{
				if (isImgValid)
					imshow(imageWindowName,img);
				else
					cout << "M2Host main loop: img is not valid." << endl;
			}

			// Timestamp related administrative things
			last2PictureTimeStamp = last1PictureTimeStamp;
			last1PictureTimeStamp = proxy.lastReceivedTimeStamp;
			timeMeasurement.finish(M2::TimeMeasurementCodeDefs::FrameAll);

			if (configManager.showImage)
			{
				// If the images are shown, have to wait to allow it to display...
				waitKey(25);
			}
		}
		cout << "Necessary pictures taken. Receiving measurement log..." << endl;
		proxy.RequestLog();

		JsonMessage *msg = NULL;
		MeasurementLogMessage *logMsg = NULL;
		Mat img;
		msg = proxy.ReceiveNew();
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

		running = false;
	}
}

void M2_TrackingTest(PhoneProxy proxy)
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
		// Request image from the phone
		timeMeasurement.start(M2::TimeMeasurementCodeDefs::FrameAll);
		timeMeasurement.start(M2::TimeMeasurementCodeDefs::Send);
		proxy.RequestPhoto(0);
		timeMeasurement.finish(M2::TimeMeasurementCodeDefs::Send);
		// Receiving picture
		timeMeasurement.start(M2::TimeMeasurementCodeDefs::WaitAndReceive);
		JsonMessage *msg = NULL;
		Mat img;
		bool isImgValid = false;
		msg = proxy.ReceiveNew();
		if (msg->getMessageType() == Jpeg)
		{
			JpegMessage *jpegMsg = NULL;
			jpegMsg = (JpegMessage *)msg;
			jpegMsg->Decode(&img);
			
			if(img.type()==CV_8UC4)	// Convert frames from CV_8UC4 to CV_8UC3
				cvtColor(img,img,CV_BGRA2BGR);

			isImgValid = true;
		}
		else if (msg->getMessageType() == MatImage)
		{
			MatImageMessage *matimgMsg = NULL;
			matimgMsg = (MatImageMessage *)msg;
			if (matimgMsg->size != 0)
			{
				matimgMsg->Decode();
				matimgMsg->getMat()->copyTo(img);	// TODO: avoid this copy...
				isImgValid = true;
			}
		}
		else
		{
			cout << "Error... received something else than JPEG... see the log for details!" << endl;
			Logger::getInstance()->Log(Logger::LOGLEVEL_ERROR,"M2Host","Received something else than JPEG image:\n");
			msg->log();
		}
		delete msg;
		msg = NULL;
		timeMeasurement.finish(M2::TimeMeasurementCodeDefs::WaitAndReceive);

		// Image processing
		OPENCV_ASSERT(img.type()==CV_8UC3,"M2_Tracking","Mat type not CV_8UC3.");





		// Showing the picture results
		if (isImgValid)
			imshow(imageWindowName,img);

		// Timestamp related administrative things
		timeMeasurement.finish(M2::TimeMeasurementCodeDefs::FrameAll);

		// If the images are shown, have to wait to allow it to display...
		// Process possible keypress
		char ch = waitKey(25);
		switch (ch)
		{
		case -1:	// Nothing pressed
			break;
		case 'f':
		case 'x':
		case 'q':
		case 's':
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
				<< "q	quit" << endl
				<< "n	no operation" << endl
				<< "c	chessboard detection" << endl
				<< "t	tracking the marker" << endl;
		}

		frameIdx++;
	}

	cout << "Necessary pictures taken. Receiving measurement log..." << endl;
	proxy.RequestLog();

	JsonMessage *msg = NULL;
	MeasurementLogMessage *logMsg = NULL;
	Mat img;
	msg = proxy.ReceiveNew();
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
	timeMeasurement.init();
	M2::TimeMeasurementCodeDefs::setnames(&timeMeasurement);

	// Waiting a little to allow other processes to init...
	// Useful if started together with CamClient which needs to start its server.
	cout << "Waiting 3s..." << endl;
#ifdef WIN32
	Sleep(3000);
#else
#error TODO: Sleep not implemented for non-Win32.
#endif

	timeMeasurement.start(M2::TimeMeasurementCodeDefs::FullExecution);

	char ip[20];
	strcpy(ip,configManager.phoneIpAddress.c_str());
	int port = configManager.phonePort;
	PhoneProxy proxy;
	cout << "Connecting..." << endl;
	proxy.Connect(ip,port);

	// --------------------------- Execute main task

	//M2_SpeedTest(proxy);
	M2_TrackingTest(proxy);

	// --------------------------- Closing...
	cout << "Disconnecting..." << endl;
	proxy.Disconnect();

	timeMeasurement.finish(M2::TimeMeasurementCodeDefs::FullExecution);

	ofstream resultFile;
	resultFile.open(configManager.localMLogFilename, ofstream::binary);
	t = time(0);   // get time now
    now = localtime( & t );
	resultFile << "--- New results at " << (now->tm_year + 1900) << "-" << (now->tm_mon + 1) << "-" << now->tm_mday
		<< " " << now->tm_hour << ":" << now->tm_min << ":" << now->tm_sec << endl;
	resultFile << "--- Main loop time measurement results:" << endl;
	timeMeasurement.showresults(&resultFile);
	resultFile << "--- Further details:" << endl;
	resultFile << "max fps: " << timeMeasurement.getmaxfps(M2::TimeMeasurementCodeDefs::FrameAll) << endl;
	resultFile << "Number of processed frames: " << frameIdx << endl;
	resultFile << "--- PhoneProxy time measurement results:" << endl;
	proxy.timeMeasurement.showresults(&resultFile);


	resultFile.flush();
	resultFile.close();

	cout << "Done." << endl;

}
