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
#include "CameraRemoteProxy.h"
#include "CameraLocalProxy.h"
#include "VideoInputFactory.h"

// Configuration, log, time measurement
#include "myconfigmanager.h"
#include "TimeMeasurement.h"
#include "../include/TimeMeasurementCodeDefines.h"
#include "FileLogger.h"

#include "TimeSyncBeacon.h"

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
void StartTimeSyncTest(CameraProxy *camProxy, int captureNum, long long startBeaconTimeUs, long long startTimeStampUs, long long initialPlusDelayUs)
{
	Logger::getInstance()->Log(Logger::LOGLEVEL_INFO,"M2Host","Silent measurement, loglevel set to WARNING");
	Logger::getInstance()->SetLogLevel(Logger::LOGLEVEL_WARNING);
	// Open measurement results file
	std::ofstream mlog;
	mlog.open(configManager.remoteMLogFilename.c_str(),std::ofstream::binary);
	// Open measurement results file
	std::ofstream tsbrVerbose;
	if (configManager.saveTsbrArray)
	{
		tsbrVerbose.open(configManager.tsbrVerboseFilename.c_str(),std::ofstream::binary);
		tsbrVerbose << "frameIdx; tsbrArray values" << endl;
	}

	TimeSyncBeacon beacon;
	Mat beaconInternalVerboseImage;

	// Take initial picture as soon as possible
	//mlog << "M2_TimeSyncTest measurement result" << endl;
	mlog << "frameIdx;desiredBTimeUs;lastBTimeUs;desiredTStampUs;lastTStampUs;"
		 << "relDesiredBTimeMs;relLastBTimeMs;relDesiredTStampMs;relLastTStampMs;"
		 << "diff_relLBT_relLTS" <<endl;
	long long desiredBeaconTimeUs = startBeaconTimeUs + initialPlusDelayUs;

	int frameIdx = 0;
	bool finished = false;
	while (!finished)
	{
		// Choose DesiredBeaconTime (+2 seconds)
		desiredBeaconTimeUs += 2000000;	// Not relative to lastBeaconTimeUs as reading may be wrong...
		// Calculate DesiredTimeStamp
		long long desiredTimeStampUs = (desiredBeaconTimeUs - startBeaconTimeUs) + startTimeStampUs;
// --- TODO: desiredTimeStampUs: is the value reasonable? If not, override to 0!
		// Take picture at desiredTimeStamp
		camProxy->CaptureImage(desiredTimeStampUs);
		// Save image timestamp
		long long lastTimeStampUs = camProxy->lastImageTakenTimestamp;
		// Read image BeaconTime
		// TODO: insert code for reading the TimeSyncBeacon here!
		long long lastBeaconTimeUs = beacon.GetTimeUsFromImage(*camProxy->lastImageTaken);

		mlog
			<< frameIdx << ";"
			<< desiredBeaconTimeUs << ";" << lastBeaconTimeUs << ";"
			<< desiredTimeStampUs << ";" << lastTimeStampUs << ";"
			<< ((desiredBeaconTimeUs-startBeaconTimeUs)/1000) << ";" << ((lastBeaconTimeUs-startBeaconTimeUs)/1000) << ";"
			<< ((desiredTimeStampUs-startTimeStampUs)/1000) << ";" << ((lastTimeStampUs-startTimeStampUs)/1000) << ";"
			<< (((lastBeaconTimeUs-startBeaconTimeUs)/1000)-((lastTimeStampUs-startTimeStampUs)/1000)) << endl;
		cout << frameIdx << ". delta: " << (((lastBeaconTimeUs-startBeaconTimeUs)/1000)-((lastTimeStampUs-startTimeStampUs)/1000)) << " ms" << endl;

		char filename[256];
		if (configManager.saveCapturedImages)
		{
			sprintf(filename,"capturedimage%d.jpg",frameIdx);
			imwrite(filename, *camProxy->lastImageTaken );
		}

		if (configManager.saveProcessedImages)
		{
			sprintf(filename,"processedimage%d.jpg",frameIdx);
			beacon.tsbr->GenerateImage();
			merge(beacon.tsbr->channel, 3, beaconInternalVerboseImage);
			beaconInternalVerboseImage += (*camProxy->lastImageTaken)*0.5;
			imwrite(filename, beaconInternalVerboseImage );
		}

		if (configManager.saveTsbrArray)
		{
			tsbrVerbose << frameIdx << "; ";
			for(int i=0; i<64; i++)
			{
				tsbrVerbose << (int)(beacon.tsbr->davidArray[i]) << ";";
				if (i % 16 == 15)
				{
					tsbrVerbose << " ";
				}
			}
			tsbrVerbose << endl;
		}

		if (frameIdx >= captureNum)
		{
			finished=true;
		}
		frameIdx++;
	}

	mlog.flush();
	mlog.close();
	if (configManager.saveTsbrArray)
	{
		tsbrVerbose.flush();
		tsbrVerbose.close();
	}
	Logger::getInstance()->SetLogLevel(Logger::LOGLEVEL_INFO);
}

void M2_TimeSyncTest_interactiveDebug(CameraProxy *camProxy, int captureNum)
{
	TimeSyncBeacon beacon;
	Mat beaconInternalVerboseImage;
	char filename[256];

	long long lastBeaconTimeUs = 0;
	long long lastTimeStampUs = 0;

	// Take initial picture as soon as possible
	camProxy->CaptureImage(0);
	Mat *image = camProxy->lastImageTaken;

	bool finished = false;
	while (!finished)
	{
		if (!camProxy->CaptureImage(0))
		{
			// Capture failed. Possibly because the remote side used an AVI file
			//	and has reached its end.
			finished = true;
			break;
		}

		imshow( "Cam image", *camProxy->lastImageTaken);  
		char ch = waitKey(25);
		switch(ch)
		{
		case 'b':	// read beacon
			lastBeaconTimeUs = beacon.GetTimeUsFromImage(*camProxy->lastImageTaken);
			imshow( "Reader in", beacon.tsbr->channel[2] );  
			beacon.tsbr->GenerateImage();
			merge(beacon.tsbr->channel, 3, beaconInternalVerboseImage);
			beaconInternalVerboseImage += (*camProxy->lastImageTaken)*0.5;
			cout << "lastBeaconTimeUs: " << lastBeaconTimeUs << endl;
			imshow( "Reader output", beaconInternalVerboseImage);  
			//imshow( "Reader output", beaconInternalVerboseImage(beacon.tsbr->getBoundingRect()) );  
			lastTimeStampUs = camProxy->lastImageTakenTimestamp;
			if (configManager.saveImageAtB)
			{
				sprintf(filename,"b_image%d.jpg",frameIdx);
				imwrite(filename, *camProxy->lastImageTaken );
			}
			break;
		case 's':
			cout << "Starting TimeSync measurement (silent mode)" << endl;
			StartTimeSyncTest(camProxy,100,lastBeaconTimeUs,lastTimeStampUs,5000000);
			cout << "TimeSync measurement finished" << endl;
			break;
		case '1':
			camProxy->SetNormalizedExposure(1);
			camProxy->SetNormalizedGain(1);
			camProxy->SetNormalizedWhiteBalance(30,30,30);
			cout << "Exposure set" << endl;
			break;
		case '2':
			camProxy->SetNormalizedExposure(5);
			camProxy->SetNormalizedGain(1);
			camProxy->SetNormalizedWhiteBalance(30,30,30);
			cout << "Exposure set" << endl;
			break;
		case '3':
			camProxy->SetNormalizedExposure(10);
			camProxy->SetNormalizedGain(1);
			camProxy->SetNormalizedWhiteBalance(30,30,30);
			cout << "Exposure set" << endl;
			break;
		case 27:
			finished=true;
			break;
		}
		frameIdx++;
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

	const bool useLocalCamera=false;

	// Setup time management
	// Waiting a little to allow other processes to init...
	// Useful if started together with CamClient which needs to start its server.
	if (!useLocalCamera)
	{
		cout << "Waiting 3s..." << endl;
		PlatformSpecifics::getInstance()->SleepMs(3000);
	}


	CameraProxy *camProxy = NULL;
	CameraRemoteProxy *camRemoteProxy = NULL;
	CameraLocalProxy *camLocalProxy = NULL;
	if (useLocalCamera)
	{
		camProxy = camLocalProxy = new CameraLocalProxy(VIDEOINPUTTYPE_PS3EYE,0);
	}
	else
	{
		camProxy = camRemoteProxy = new CameraRemoteProxy();
	}

	// Prepare camera and detector objects
	//ChessboardDetector detector(Size(9,6),36.1);	// Chessboard cell size is 36x36mm, using CameraProxy default
	camProxy->camera->cameraID=0;
	camProxy->camera->isStationary=false;
	camProxy->camera->loadCalibrationData(configManager.camIntrinsicParamsFileName.data());

	if (camRemoteProxy)
	{
		cout << "Connecting..." << endl;
		camRemoteProxy->Connect(configManager.phoneIpAddress.c_str(),configManager.phonePort);
	}

	// --------------------------- Execute main task
	cout << "Main task started" << endl;

	// ---Currently, multiple possible measurements are supported.
	// - Original "capture 100 frames and report capture times" measurement
	//camRemoteProxy->PerformCaptureSpeedMeasurement_A(100,configManager.MLogFilename.c_str());
	// - Reading TimeSyncBeacon measurement
	//M2_TimeSyncTest(camProxy,10);
	M2_TimeSyncTest_interactiveDebug(camProxy,10);

	cout << "Main task finished" << endl;
	// --------------------------- Closing...
	if (camRemoteProxy)
	{
		cout << "Disconnecting..." << endl;
		camRemoteProxy->Disconnect();
	}

	delete camProxy;
	cout << "Done." << endl;
}
