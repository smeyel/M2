#include <winsock2.h>
#include <ws2tcpip.h>
#include <sstream>	// for ostringstream (assembly of answers)
#include <fstream>

#include <opencv2/opencv.hpp>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>

#include "Logger.h"
#include "TimeMeasurementCodeDefines.h"
#include "TimeMeasurement.h"
#include "JsonMessage.h"
#include "JpegMessage.h"
#include "PingMessage.h"
#include "SendlogMessage.h"
#include "TakePictureMessage.h"
#include "VideoInputFactory.h"
#include "MeasurementLogMessage.h"
#include "MatImageMessage.h"


#include "PhoneServer.h"

#include "myconfigmanager.h"
#include "globals.h"

using namespace cv;
using namespace std;
using namespace LogConfigTime;

extern Mat *frameCaptured;
extern VideoInput *videoInput;
extern int imageNumber;
extern bool running;

void handlePing(PingMessage *msg, PhoneServer *server)
{
	PingMessage ping;
	server->Send(&ping);
}

void handleTakePicture(TakePictureMessage *msg, PhoneServer *server)
{
	timeMeasurement.start(CamClient::TimeMeasurementCodeDefs::WaitForTimestamp);
	if (msg->desiredtimestamp > 0)
	{
		long long currentTimeStamp = timeMeasurement.getTimeStamp();
		while( currentTimeStamp < msg->desiredtimestamp )
		{
			// Sleep length: calculated time minus 50ms for safety...
			long sleepMilliSec = (long)((msg->desiredtimestamp - currentTimeStamp) / 1000 - 50);
			Logger::getInstance()->Log(Logger::LOGLEVEL_VERBOSE,"CamClient","Waiting, sleepMilliSec = %Ld\n",sleepMilliSec);
			if (sleepMilliSec > 0)
			{
#ifdef WIN32
				Sleep(sleepMilliSec);
#else
#error TODO: Sleep not implemented for non-Win32.
#endif
			}
			currentTimeStamp = timeMeasurement.getTimeStamp();
		}
	}
	Logger::getInstance()->Log(Logger::LOGLEVEL_VERBOSE,"CamClient","Now taking picture...\n");
	timeMeasurement.finish(CamClient::TimeMeasurementCodeDefs::WaitForTimestamp);

	// Taking a picture
	timeMeasurement.start(CamClient::TimeMeasurementCodeDefs::Capture);
	long long timestamp = timeMeasurement.getTimeStamp();
	videoInput->captureFrame(*frameCaptured);
	timeMeasurement.finish(CamClient::TimeMeasurementCodeDefs::Capture);

	if (configManager.sendMatImage)
	{
		// Sending images in openCV Mat format
		MatImageMessage ans;
		ans.timestamp = timestamp;
		ans.Encode(frameCaptured);

		ans.log();
		if (configManager.showResponseOnCout)
			cout << "{ \"type\":\"MatImage\", \"timestamp\":\"" << ans.timestamp << "\", \"size\":\"" << ans.size << "\" }#" << endl;

		// Sending the answer and the JPEG encoded picture
		timeMeasurement.start(CamClient::TimeMeasurementCodeDefs::AnsweringTakePicture);
		server->Send(&ans);
		timeMeasurement.finish(CamClient::TimeMeasurementCodeDefs::AnsweringTakePicture);
	}
	else	// Sending JPEG compressed images
	{
		JpegMessage jpegMsg;
		jpegMsg.timestamp = timestamp;

		// JPEG compression
		timeMeasurement.start(CamClient::TimeMeasurementCodeDefs::JpegCompression);
		jpegMsg.Encode(frameCaptured);
		timeMeasurement.finish(CamClient::TimeMeasurementCodeDefs::JpegCompression);

		// Assembly of the answer
		timeMeasurement.start(CamClient::TimeMeasurementCodeDefs::AnsweringTakePicture);
		if (configManager.showResponseOnCout)
			cout << "{ \"type\":\"JPEG\", \"timestamp\":\"" << jpegMsg.timestamp << "\", \"size\":\"" << jpegMsg.size << "\" }#" << endl;

		// Sending the answer and the JPEG encoded picture
		server->Send(&jpegMsg);
		timeMeasurement.finish(CamClient::TimeMeasurementCodeDefs::AnsweringTakePicture);

		// Showing the image after sending it, so that it causes smaller delay...
		if (configManager.showImage)
		{
			timeMeasurement.start(CamClient::TimeMeasurementCodeDefs::ShowImage);
			Mat show = imdecode(Mat(jpegMsg.data),CV_LOAD_IMAGE_COLOR); 
			imshow(imageWindowName,show);
			int key = waitKey(25);	// TODO: needs some kind of delay to show!!!
			timeMeasurement.finish(CamClient::TimeMeasurementCodeDefs::ShowImage);
		}
	}

	imageNumber++;
}

void handleSendlog(SendlogMessage *msg, PhoneServer *server)
{
	timeMeasurement.start(CamClient::TimeMeasurementCodeDefs::AnsweringSendlog);
	MeasurementLogMessage answerMsg;

	// Create the log
	std::ostringstream measurementLog;
	time_t t = time(0);   // get time now
    struct tm * now = localtime( & t );
	measurementLog << "--- New results at " << (now->tm_year + 1900) << "-" << (now->tm_mon + 1) << "-" << now->tm_mday
		<< " " << now->tm_hour << ":" << now->tm_min << ":" << now->tm_sec << endl;
	measurementLog << "Ini file: " << inifilename << endl;
	measurementLog << "Log file: " << configManager.logFileName << endl;
	measurementLog << "--- Main loop time measurement results:" << endl;
	timeMeasurement.showresults(&measurementLog);
	measurementLog << "--- Further details:" << endl;
	measurementLog << "Number of captured images: " << imageNumber << endl;
	measurementLog << "--- End of results" << endl;
	std::string measurementLogString = measurementLog.str();

	// Send
	answerMsg.timestamp = timeMeasurement.getTimeStamp();
	answerMsg.size = measurementLogString.length();
	const char *ptr = measurementLogString.c_str();
	for(int i=0; i<answerMsg.size; i++)
	{
		answerMsg.data.push_back(ptr[i]);
	}
	server->Send(&answerMsg);
	timeMeasurement.finish(CamClient::TimeMeasurementCodeDefs::AnsweringSendlog);
}

void handleJSON(JsonMessage *msg, PhoneServer *server)
{
	Logger::getInstance()->Log(Logger::LOGLEVEL_INFO,"CamClient","Received command:\n");
	msg->log();

	switch(msg->getMessageType())
	{
	case Default:
		Logger::getInstance()->Log(Logger::LOGLEVEL_WARNING,"CamClient","Handling default message, nothing to do.\n");
		break;
	case Ping:
		handlePing((PingMessage*)msg,server);
		break;
	case TakePicture:
		timeMeasurement.start(CamClient::TimeMeasurementCodeDefs::HandleTakePicture);
		handleTakePicture((TakePictureMessage*)msg,server);
		timeMeasurement.finish(CamClient::TimeMeasurementCodeDefs::HandleTakePicture);
		break;
	case Sendlog:
		handleSendlog((SendlogMessage*)msg,server);
		break;
	default:
		Logger::getInstance()->Log(Logger::LOGLEVEL_WARNING,"CamClient","Unknown message type!\n");
	}

	Logger::getInstance()->Log(Logger::LOGLEVEL_INFO,"CamClient","Command handled\n");
	return;
}
