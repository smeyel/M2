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
#include "PingMessage.h"
#include "SendlogMessage.h"
#include "TakePictureMessage.h"
#include "VideoInputFactory.h"

#include "myconfigmanager.h"
#include "globals.h"

using namespace cv;
using namespace std;
using namespace LogConfigTime;

extern Mat *frameCaptured;
extern VideoInput *videoInput;
extern int imageNumber;
extern bool running;

//#include "picojson.h"

void handlePing(PingMessage *msg, SOCKET sock)
{
	char *response = "{ \"type\"=\"pong\" }";
	int n = send(sock,response,strlen(response),0);
	if (n < 0)
	{
		Logger::getInstance()->Log(Logger::LOGLEVEL_ERROR,"CamClient","Error on writing answer to socket.\n");
	}
}

void handleTakePicture(TakePictureMessage *msg, SOCKET sock)
{
	timeMeasurement.start(CamClient::TimeMeasurementCodeDefs::WaitForTimestamp);
	if (msg->desiredtimestamp > 0)
	{
		long long currentTimeStamp = timeMeasurement.getTimeStamp();
		while( currentTimeStamp < msg->desiredtimestamp )
		{
			// Sleep length: calculated time minus 50ms for safety...
			long sleepMilliSec = (msg->desiredtimestamp - currentTimeStamp) / 1000 - 50;
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

	// JPEG compression
	timeMeasurement.start(CamClient::TimeMeasurementCodeDefs::JpegCompression);
	vector<uchar> buff;//buffer for coding
	vector<int> param = vector<int>(2);
	param[0]=CV_IMWRITE_JPEG_QUALITY;
	param[1]=95;//default(95) 0-100

	imencode(".jpg",*frameCaptured,buff,param);
	timeMeasurement.finish(CamClient::TimeMeasurementCodeDefs::JpegCompression);
	
	// Assembly of the answer
	timeMeasurement.start(CamClient::TimeMeasurementCodeDefs::AnsweringTakePicture);
	long filesize = buff.size();
	std::ostringstream out;
	out << "{ \"type\":\"JPEG\", \"timestamp\":\"" << timestamp << "\", \"size\":\"" << filesize << "\" }#";

	// Sending the answer and the JPEG encoded picture
	std::string tmpStr = out.str();
	const char* tmpPtr = tmpStr.c_str();

	int n = send(sock,tmpPtr,strlen(tmpPtr),0);
	if (n < 0)
	{
		Logger::getInstance()->Log(Logger::LOGLEVEL_ERROR,"CamClient","Error on writing answer to socket.\n");
	}

	n = send(sock,(const char*)(buff.data()),buff.size(),0);
	if (n < 0)
	{
		Logger::getInstance()->Log(Logger::LOGLEVEL_ERROR,"CamClient","Error on writing answer to socket.\n");
	}
	timeMeasurement.finish(CamClient::TimeMeasurementCodeDefs::AnsweringTakePicture);

	// Showing the image after sending it, so that it causes smaller delay...
	if (configManager.showImage)
	{
		timeMeasurement.start(CamClient::TimeMeasurementCodeDefs::ShowImage);
		Mat show = imdecode(Mat(buff),CV_LOAD_IMAGE_COLOR); 
		imshow(imageWindowName,show);
		int key = waitKey(25);	// TODO: needs some kind of delay to show!!!
		timeMeasurement.finish(CamClient::TimeMeasurementCodeDefs::ShowImage);
	}

	imageNumber++;
}

void handleSendlog(SendlogMessage *msg, SOCKET sock)
{
	timeMeasurement.start(CamClient::TimeMeasurementCodeDefs::AnsweringSendlog);

	// Create the log
	std::ostringstream measurementLog;
	// Write results to the results file
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

	// TODO: timestamp
	long long timestamp = timeMeasurement.getTimeStamp();
	long filesize = measurementLogString.length();
	std::ostringstream answer;
	answer << "{ \"type\":\"measurementlog\", \"timestamp\":\"" << timestamp << "\", \"size\":\"" << filesize << "\" }#";
	answer << measurementLogString;

	// Sending the answer and the JPEG encoded picture
	std::string tmpStr = answer.str();
	const char* tmpPtr = tmpStr.c_str();

	int n = send(sock,tmpPtr,strlen(tmpPtr),0);
	if (n < 0)
	{
		Logger::getInstance()->Log(Logger::LOGLEVEL_ERROR,"CamClient","Error on writing answer to socket.\n");
	}
	timeMeasurement.finish(CamClient::TimeMeasurementCodeDefs::AnsweringSendlog);
}

void handleJSON(char *json, SOCKET sock)
{
	timeMeasurement.start(CamClient::TimeMeasurementCodeDefs::ReadJson);
	JsonMessage *msg = JsonMessage::parse(json);
	timeMeasurement.finish(CamClient::TimeMeasurementCodeDefs::ReadJson);

	Logger::getInstance()->Log(Logger::LOGLEVEL_INFO,"CamClient","Received command:\n");
	msg->log();

	switch(msg->getMessageType())
	{
	case Default:
		Logger::getInstance()->Log(Logger::LOGLEVEL_WARNING,"CamClient","Handling default message, nothing to do.\n");
		break;
	case Ping:
		handlePing((PingMessage*)msg,sock);
		break;
	case TakePicture:
		timeMeasurement.start(CamClient::TimeMeasurementCodeDefs::HandleTakePicture);
		handleTakePicture((TakePictureMessage*)msg,sock);
		timeMeasurement.finish(CamClient::TimeMeasurementCodeDefs::HandleTakePicture);
		break;
	case Sendlog:
		handleSendlog((SendlogMessage*)msg,sock);
		break;
	default:
		Logger::getInstance()->Log(Logger::LOGLEVEL_WARNING,"CamClient","Unknown message type!\n");
	}

	Logger::getInstance()->Log(Logger::LOGLEVEL_INFO,"CamClient","Command handled\n");
	return;
}

