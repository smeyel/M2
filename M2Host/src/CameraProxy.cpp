#include <fstream>
#include "CameraProxy.h"

#include "Logger.h"
#include "JpegMessage.h"
#include "MatImageMessage.h"

#include "MeasurementLogMessage.h"

using namespace LogConfigTime;

CameraProxy::CameraProxy(PhoneProxy *aPhoneProxy, Camera *aCamera)
{
	phoneproxy = aPhoneProxy;
	camera = aCamera;

	initDefaults();
}

CameraProxy::CameraProxy()
{
	initDefaults();
}

void CameraProxy::initDefaults()
{
	lastImageTaken = new Mat();

	phoneproxy = default_phoneproxy = new PhoneProxy();
	camera = default_camera = new Camera();
	chessboarddetector = default_chessboarddetector = new ChessboardDetector(Size(9,6),36.1);
}

CameraProxy::~CameraProxy()
{
	if (lastImageTaken)
		delete lastImageTaken;
	lastImageTaken=NULL;

	if (phoneproxy == default_phoneproxy)
		phoneproxy = NULL;
	if (default_phoneproxy)
		delete default_phoneproxy;
	default_phoneproxy=NULL;

	if (camera == default_camera)
		camera = NULL;
	if (default_camera)
		delete default_camera;
	default_camera=NULL;

	if (chessboarddetector == default_chessboarddetector)
		chessboarddetector = NULL;
	if (default_chessboarddetector)
		delete default_chessboarddetector;
	default_chessboarddetector=NULL;
}

void CameraProxy::Connect(const char *ip, int port)
{
	phoneproxy->Connect(ip,port);
}

void CameraProxy::Disconnect()
{
	phoneproxy->Disconnect();
}


void CameraProxy::CaptureImage(long long desiredTimestamp)
{
	CaptureImage(desiredTimestamp, lastImageTaken);
}

// CaptureImage
void CameraProxy::CaptureImage(long long desiredTimestamp, Mat *target)
{
	// Request image from the phone
	phoneproxy->RequestPhoto(desiredTimestamp);
	// Receiving picture
	JsonMessage *msg = NULL;
	bool isImgValid = false;
	msg = phoneproxy->ReceiveNew();
	if (msg->getMessageType() == Jpeg)
	{
		JpegMessage *jpegMsg = NULL;
		jpegMsg = (JpegMessage *)msg;
		jpegMsg->Decode(target);

		if (target==lastImageTaken)
			lastImageTakenTimestamp = jpegMsg->timestamp;
			
		if(target->type()==CV_8UC4)	// Convert frames from CV_8UC4 to CV_8UC3
			cvtColor(*target,*target,CV_BGRA2BGR);

		isImgValid = true;
	}
	else if (msg->getMessageType() == MatImage)
	{
		MatImageMessage *matimgMsg = NULL;
		matimgMsg = (MatImageMessage *)msg;
		if (matimgMsg->size != 0)
		{
			matimgMsg->Decode();
			matimgMsg->getMat()->copyTo(*target);	// TODO: avoid this copy...

			if (target==lastImageTaken)
				lastImageTakenTimestamp = matimgMsg->timestamp;

			isImgValid = true;
		}
	}
	else
	{
		cout << "Error... received something else than JPEG or MAT... see the log for details!" << endl;
		Logger::getInstance()->Log(Logger::LOGLEVEL_ERROR,"M2Host","Received something else than JPEG image:\n");
		msg->log();
	}
	delete msg;
	msg = NULL;
}

// Capture image and try to find chessboard
bool CameraProxy::CaptureAndTryCalibration(bool showResultOnImage)
{
	CaptureImage();
	return TryCalibration(showResultOnImage);
}

bool CameraProxy::TryCalibration(bool showResultOnImage)
{
	return TryCalibration(lastImageTaken, showResultOnImage);
}

bool CameraProxy::TryCalibration(Mat *image, bool showResultOnImage)
{
	if (chessboarddetector->findChessboardInFrame(*image))
	{
		camera->calculateExtrinsicParams(chessboarddetector->chessboard.corners,chessboarddetector->pointBuf);

		// Show calibration data on the frame
		if (showResultOnImage)
		{
			drawChessboardCorners(*image,Size(9,6),chessboarddetector->pointBuf,true);
			Matx44f T = camera->GetT();
			for(int i=0; i<16; i++)
			{
				char txt[50];
				sprintf(txt, "%4.2lf",T.val[i] );
				putText( *lastImageTaken, string(txt), cvPoint( 25+(i%4)*75, 20+(i/4)*20 ), FONT_HERSHEY_DUPLEX, 0.5, CV_RGB(255,255,0) );
			}
		}
		return true;
	}
	if (showResultOnImage)
	{
		putText( *lastImageTaken, string("Cannot find chessboard"), cvPoint( 25, 20 ), FONT_HERSHEY_DUPLEX, 0.5, CV_RGB(255,100,0) );
	}
	return false;
}


// Ask for images until calibration is successful
bool CameraProxy::CaptureUntilCalibrated(int maxFrameNum)
{
	for(int i=0; i<maxFrameNum; i++)
	{
		if (CaptureAndTryCalibration())
			return true;
	}
	return false;
}

// Return Ray3D for given image coordinates
Ray CameraProxy::pointImg2World(Point2f pImg)
{
	return camera->pointImg2World(pImg);
}

// Warning: results are influenced by remote side settings, like send Jpeg or Mat!
void CameraProxy::PerformCaptureSpeedMeasurement_A(int frameNumber, const char *resultfilename)
{
	if (!resultfilename)
	{
		Logger::getInstance()->Log(Logger::LOGLEVEL_ERROR,"CameraProxy","PerformCaptureSpeedMeasurement_A result filename not set.\n");
		return;
	}

	// Create local time measurement object
	const int TimeFrame				= 0;	// Processing a frame
	const int TimeSend				= 1;	// Send image request and wait for reception
	const int TimeWaitAndReceive	= 2;	// Receiving the image
	const int TimeProcessImage		= 3;
	const int TimeFullExecution		= 4;	// Full execution time of the measurement

	TimeMeasurement timeMeasurement;
	timeMeasurement.setMeasurementName("CameraProxy.PerformCaptureSpeedMeasurement_A");
	timeMeasurement.setname(TimeFrame, "TimeFrameAll");
	timeMeasurement.setname(TimeSend, "TimeSend");
	timeMeasurement.setname(TimeWaitAndReceive, "TimeWaitAndReceive");
	timeMeasurement.setname(TimeProcessImage, "TimeProcessImage");
	timeMeasurement.setname(TimeFullExecution, "TimeFullExecution");
	timeMeasurement.init();

	timeMeasurement.start(TimeFullExecution);
	for(int i=0; i<frameNumber; i++)
	{
		timeMeasurement.start(TimeFrame);
		// Asking for a picture (as soon as possible)
		timeMeasurement.start(TimeSend);
		phoneproxy->RequestPhoto(0);
		timeMeasurement.finish(TimeSend);

		// Receiving picture
		timeMeasurement.start(TimeWaitAndReceive);

		JsonMessage *msg = NULL;
		Mat img;
		bool isImgValid = false;
		msg = phoneproxy->ReceiveNew();
		timeMeasurement.finish(TimeWaitAndReceive);
		timeMeasurement.start(TimeProcessImage);
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
		timeMeasurement.finish(TimeProcessImage);
		timeMeasurement.finish(TimeFrame);
	}
	timeMeasurement.finish(TimeFullExecution);

	ofstream resultFile;
	resultFile.open(resultfilename, ios::out | ios::app );
	time_t t = time(0);   // get time now
    struct tm * now = localtime( & t );
	resultFile << "===== PerformCaptureSpeedMeasurement_A results =====" << endl << "Measurement time: " << (now->tm_year + 1900) << "-" << (now->tm_mon + 1) << "-" << now->tm_mday
		<< " " << now->tm_hour << ":" << now->tm_min << ":" << now->tm_sec << endl;

	resultFile << "--- REMOTE measurement log ---" << endl;

	// Write remote measurement log into file
	phoneproxy->RequestLog();
	JsonMessage *msg = NULL;
	MeasurementLogMessage *logMsg = NULL;
	Mat img;
	msg = phoneproxy->ReceiveNew();
	if (msg->getMessageType() == MeasurementLog)
	{
		logMsg = (MeasurementLogMessage *)msg;
		logMsg->writeAuxStream(&resultFile);
	}
	else
	{
		cout << "Error... received something else than a measurement log... see the log for details!" << endl;
		Logger::getInstance()->Log(Logger::LOGLEVEL_ERROR,"M2Host","Received something else than JPEG image:\n");
		msg->log();
	}

	// Append local measurement log to the file
	resultFile << "--- LOCAL PerformCaptureSpeedMeasurement_A measurement log ---" << endl;
	timeMeasurement.showresults(&resultFile);
	resultFile << "--- Further details:" << endl;
	resultFile << "max fps: " << timeMeasurement.getmaxfps(TimeFrame) << endl;
	resultFile << "Number of processed frames: " << frameNumber << endl;
	resultFile << "--- LOCAL PhoneProxy time measurement log ---" << endl;
	phoneproxy->timeMeasurement.showresults(&resultFile);

	resultFile.flush();
	resultFile.close();

	Logger::getInstance()->Log(Logger::LOGLEVEL_INFO,"CameraProxy","PerformCaptureSpeedMeasurement_A results written to %s\n.",resultfilename);
}
