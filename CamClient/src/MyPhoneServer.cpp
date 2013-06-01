#include "MyPhoneServer.h"

// For showing an image with imshow
#include <opencv2/opencv.hpp>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>

#include "MeasurementLogMessage.h"
#include "MatImageMessage.h"
#include "JpegMessage.h"
#include "StdoutLogger.h"

const char *MyPhoneServer::imageWindowName = "JPEG";

void MyPhoneServer::init(char *inifilename, int argc, char **argv)
{
	// Init config, allows overrides (including the ini file name)
	configManager.init(inifilename, argc, argv);

	if (configManager.camID>=0)
	{
		camProxy = new CameraLocalProxy(
			configManager.usePs3eye ? VIDEOINPUTTYPE_PS3EYE : VIDEOINPUTTYPE_GENERIC,
			configManager.camID);
	}
	else
	{
		// Use given filename to create file based camera
		camProxy = new CameraLocalProxy(configManager.camSourceFilename.data());
	}

	// Init logger (singleton)
	Logger *logger = new StdoutLogger();
	//logger->SetLogLevel(Logger::LOGLEVEL_INFO);
	logger->SetLogLevel(Logger::LOGLEVEL_ERROR);
	Logger::getInstance()->Log(Logger::LOGLEVEL_VERBOSE,"CamClient","CamClient started\n");
    time_t t = time(0);   // get time now
    struct tm * now = localtime( & t );
	Logger::getInstance()->Log(Logger::LOGLEVEL_INFO,"CamClient","Current time: %d-%d-%d, %d:%d:%d\n",
		(now->tm_year + 1900),(now->tm_mon + 1),now->tm_mday,now->tm_hour,now->tm_min,now->tm_sec );
	Logger::getInstance()->Log(Logger::LOGLEVEL_VERBOSE,"CamClient","Ini file: %s\n",inifilename);

	// Initialize socket communication and server socket
	InitServer(configManager.serverPort);

	if (configManager.showImage)
	{
		cv::namedWindow(imageWindowName, CV_WINDOW_AUTOSIZE);
	}
}


JsonMessage *MyPhoneServer::PingCallback(PingMessage *msg)
{
	PingMessage *ping = new PingMessage();
	return ping;
}

JsonMessage *MyPhoneServer::TakePictureCallback(TakePictureMessage *msg)
{
	JsonMessage *answer = NULL;
	timeMeasurement.start(TimeMeasurementCodeDefs::ImageCapture);
	camProxy->CaptureImage(msg->desiredtimestamp);
	timeMeasurement.finish(TimeMeasurementCodeDefs::ImageCapture);

	if (configManager.sendMatImage)
	{
		// Sending images in openCV Mat format
		MatImageMessage *matAnswer = new MatImageMessage();
		matAnswer->timestamp = camProxy->lastImageTakenTimestamp;
		matAnswer->Encode(camProxy->lastImageTaken);

		matAnswer->log();
		if (configManager.showResponseOnCout)
			cout << "{ \"type\":\"MatImage\", \"timestamp\":\"" << matAnswer->timestamp << "\", \"size\":\"" << matAnswer->size << "\" }#" << endl;

		// Sending the answer and the JPEG encoded picture
		answer = matAnswer;
	}
	else	// Sending JPEG compressed images
	{
		JpegMessage *jpegAnswer = new JpegMessage();
		jpegAnswer->timestamp = camProxy->lastImageTakenTimestamp;

		// JPEG compression
		timeMeasurement.start(TimeMeasurementCodeDefs::JpegCompression);
		jpegAnswer->Encode(camProxy->lastImageTaken);
		timeMeasurement.finish(TimeMeasurementCodeDefs::JpegCompression);

		// Assembly of the answer
		if (configManager.showResponseOnCout)
			cout << "{ \"type\":\"JPEG\", \"timestamp\":\"" << jpegAnswer->timestamp << "\", \"size\":\"" << jpegAnswer->size << "\" }#" << endl;

		// Sending the answer and the JPEG encoded picture
		answer = jpegAnswer;

		// Showing the image after sending it, so that it causes smaller delay...
		if (configManager.showImage)
		{
			timeMeasurement.start(TimeMeasurementCodeDefs::ShowImage);
			cv::Mat show = cv::imdecode(cv::Mat(jpegAnswer->data),CV_LOAD_IMAGE_COLOR); 
			cv::imshow(imageWindowName,show);
			int key = cv::waitKey(25);	// TODO: needs some kind of delay to show!!!
			timeMeasurement.finish(TimeMeasurementCodeDefs::ShowImage);
		}
	}
	imageNumber++;
	return answer;
}

JsonMessage *MyPhoneServer::SendPositionCallback(SendPositionMessage *msg)
{
	imageNumber++;
	// TODO
	return NULL;
}

JsonMessage *MyPhoneServer::SendLogCallback(SendlogMessage *msg)
{
	MeasurementLogMessage *answer = new MeasurementLogMessage();

	// Create the log
	std::ostringstream measurementLog;
	time_t t = time(0);   // get time now
    struct tm * now = localtime( & t );
	measurementLog << "--- New results at " << (now->tm_year + 1900) << "-" << (now->tm_mon + 1) << "-" << now->tm_mday
		<< " " << now->tm_hour << ":" << now->tm_min << ":" << now->tm_sec << endl;
	measurementLog << "Log file: " << configManager.logFileName << endl;
	measurementLog << "--- Main loop time measurement results:" << endl;
	timeMeasurement.showresults(&measurementLog);
	measurementLog << "--- Further details:" << endl;
	measurementLog << "Number of captured images: " << imageNumber << endl;
	measurementLog << "--- End of results" << endl;
	std::string measurementLogString = measurementLog.str();

	// Send
	answer->timestamp = timeMeasurement.getTimeStamp();
	answer->size = measurementLogString.length();
	const char *ptr = measurementLogString.c_str();
	for(int i=0; i<answer->size; i++)
	{
		answer->data.push_back(ptr[i]);
	}
	return answer;
}
