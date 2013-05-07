#include <iostream>
#include <sstream>
#include <time.h>
#include <fstream>	// For marker data export into file...

#include <windows.h>	// for sleep

#include <opencv2/opencv.hpp>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>

#include "../include/PhoneProxy.h"

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
char *configfilename = "m2_default.ini";	// 1st command line parameter overrides it (if exists)

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

	// Setup time management
	LogConfigTime::TimeMeasurement timeMeasurement;
	timeMeasurement.init();
	M2::TimeMeasurementCodeDefs::setnames(&timeMeasurement);

	timeMeasurement.start(M2::TimeMeasurementCodeDefs::FullExecution);
	bool running=true;
	int frameIdx = 0;
	while(running)
	{

		// Request image from the phone

		//char *ip = "152.66.173.130";
		char ip[20];
		strcpy(ip,configManager.phoneIpAddress.c_str());
		int port = configManager.phonePort;
		PhoneProxy proxy;

		char tmpBuff[100];
		/*cout << "Press enter to start connecting..." << endl;
		cin >> tmpBuff;*/

/*		proxy.Connect(ip,port);
		proxy.RequestPing();
		proxy.ReceiveDebug();
		//proxy.Receive("d:\\temp\\nothing.txt");
		proxy.Disconnect();

		continue;	// WARNING, function bypass! */

		_int64 desiredTimeStamp = 0;
		_int64 last1PictureTimeStamp = 0;	// Last timestamp
		_int64 last2PictureTimeStamp = 0;	// Timestamp before the last one
		_int64 interPictureTime = 0;

		for(int i=0; i<55; i++)	// First 2 photos do not have desired timestamp...
		{
			// Calculate desiredTimeStamp
			if (interPictureTime==0 && last2PictureTimeStamp>0)
			{
				// Calculate time between the first two images
				interPictureTime = last1PictureTimeStamp - last2PictureTimeStamp;
			}

			if (interPictureTime==0)
			{
				// Unknown inter-picture time (we do not know the frequency of the timestamp!!!)
				desiredTimeStamp = 0;
			}
			else
			{
				desiredTimeStamp = last1PictureTimeStamp + interPictureTime * 2;
			}

			// Asking for a picture
			timeMeasurement.start(M2::TimeMeasurementCodeDefs::FrameAll);
			cout << "Capture No " << i << "..." << endl;
			cout << "Connecting..." << endl;
			proxy.Connect(ip,port);
			cout << "Requesing photo..." << endl;
			timeMeasurement.start(M2::TimeMeasurementCodeDefs::Send);
			proxy.RequestPhoto(desiredTimeStamp);
			timeMeasurement.finish(M2::TimeMeasurementCodeDefs::Send);
			cout << "Receiving photo..." << endl;
			timeMeasurement.start(M2::TimeMeasurementCodeDefs::WaitAndReceive);
			proxy.Receive("d:\\temp\\image1.jpg");
			//proxy.ReceiveDebug();
			timeMeasurement.finish(M2::TimeMeasurementCodeDefs::WaitAndReceive);
			last2PictureTimeStamp = last1PictureTimeStamp;
			last1PictureTimeStamp = proxy.lastReceivedTimeStamp;
			cout << "Disconnecting..." << endl;
			proxy.Disconnect();
			timeMeasurement.finish(M2::TimeMeasurementCodeDefs::FrameAll);
			Sleep(500);
		}

		/*cout << "Connecting..." << endl;
		proxy.Connect(ip,port);
		cout << "Requesing photo..." << endl;
		timeMeasurement.start(M2::TimeMeasurementCodeDefs::Send);
		proxy.RequestPhoto(0);
		timeMeasurement.finish(M2::TimeMeasurementCodeDefs::Send);
		cout << "Receiving photo..." << endl;
		timeMeasurement.start(M2::TimeMeasurementCodeDefs::WaitAndReceive);
		proxy.Receive("d:\\temp\\image2.jpg");
		timeMeasurement.finish(M2::TimeMeasurementCodeDefs::WaitAndReceive);
		cout << "Disconnecting..." << endl;
		proxy.Disconnect();*/

		cout << "Done..." << endl;

		running = false;

		frameIdx++;
	}
	timeMeasurement.finish(M2::TimeMeasurementCodeDefs::FullExecution);
/*	Logger::getInstance()->Log(Logger::LOGLEVEL_VERBOSE,"M2Host","Current time\n"
	log << "--- Main loop time measurement results:" << endl;
	timeMeasurement.showresults(&log);

	log << "--- Further details:" << endl;
	log << "max fps: " << timeMeasurement.getmaxfps(M2::TimeMeasurementCodeDefs::FrameAll) << endl;
	log << "Number of processed frames: " << frameIdx << endl;

	log.flush();
	log.close();*/

	cout << "Done." << endl;
}
