// This program creates a phone-like SMEyeL node using a local camera, like a Ps3Eye.
// It communicates just like another smartphone, through the Framework.libCommunication.PhoneProxy.

#include <opencv2/opencv.hpp>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>

// For socket communication
#include <winsock2.h>
#include <ws2tcpip.h>
#include <sstream>	// for ostringstream (assembly of answers)
#include <fstream>

#include "myconfigmanager.h"
#include "TimeMeasurementCodeDefines.h"

#include "VideoInputFactory.h"

#include "myconfigmanager.h"

#include "Logger.h"
//#include "FileLogger.h"
#include "StdOutLogger.h"

#include "JsonMessage.h"
#include "PingMessage.h"
#include "TakePictureMessage.h"

#include "picojson.h"

using namespace cv;
using namespace std;
using namespace LogConfigTime;

char *inifilename = "default.ini";	// 1st command line parameter overrides it

const char *imageWindowName = "CamClient JPEG";

MyConfigManager configManager;
TimeMeasurement timeMeasurement;
Mat *frameCaptured;
SOCKET serversock;
VideoInput *videoInput;
int imageNumber=0;	// Counts the number of images taken
bool running=true;	// Used to finish the main loop...

bool takePicture(VideoInput *videoInput, Mat *frameCaptured)
{
	videoInput->captureFrame(*frameCaptured);
	if (frameCaptured->empty())
	{
		Logger::getInstance()->Log(Logger::LOGLEVEL_INFO,"CamClient","End of video\n");
		return false;
	}
	return true;
}

void initServerSocket(int port)
{
	struct sockaddr_in server;
    struct hostent *host_info;
    unsigned long addr;
	int iResult;

    WORD wVersionRequested;
    WSADATA wsaData;
    wVersionRequested = MAKEWORD (1, 1);
    if (WSAStartup (wVersionRequested, &wsaData) != 0)
	{
		Logger::getInstance()->Log(Logger::LOGLEVEL_ERROR,"CamClient","Error: Cannot initialize winsock!\n");
		exit(1);
	}

    serversock = socket( AF_INET, SOCK_STREAM, 0 );
    if (serversock < 0)
	{
		Logger::getInstance()->Log(Logger::LOGLEVEL_ERROR,"CamClient","Error: Cannot create server socket!\n");
		exit(2);
	}

    memset( &server, 0, sizeof (server));
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = INADDR_ANY;
	server.sin_port = htons(port);

	// Binding the socket
	if (bind(serversock, (struct sockaddr *) &server, sizeof(server)) < 0)
	{
		Logger::getInstance()->Log(Logger::LOGLEVEL_ERROR,"CamClient","Error: Cannot bind server socket!\n");
		exit(3);
	}
	Logger::getInstance()->Log(Logger::LOGLEVEL_INFO,"CamClient","Server socket initialized on port %d\n",port);

}

void openServerSocket()
{
	listen(serversock,5);
	Logger::getInstance()->Log(Logger::LOGLEVEL_INFO,"CamClient","Server socket now listening.\n");
}

void closeServerSocket()
{
	closesocket(serversock);
	WSACleanup();
	serversock = -1;
}

void init(char *inifilename)
{
	// Init config
	configManager.init(inifilename);

	// Init logger (singleton)
	Logger *logger = new StdoutLogger();
	logger->SetLogLevel(Logger::LOGLEVEL_INFO);
	Logger::getInstance()->Log(Logger::LOGLEVEL_VERBOSE,"CamClient","CamClient started\n");
	Logger::getInstance()->Log(Logger::LOGLEVEL_VERBOSE,"CamClient","Ini file: %s\n",inifilename);

	// Init time measurement
	timeMeasurement.init();
	CamClient::TimeMeasurementCodeDefs::setnames(&timeMeasurement);

	// Initialize camera
	if (configManager.usePs3eye)
	{
		videoInput = VideoInputFactory::CreateVideoInput(VIDEOINPUTTYPE_PS3EYE);
	}
	else
	{
		videoInput = VideoInputFactory::CreateVideoInput(VIDEOINPUTTYPE_GENERIC);
	}
	if (configManager.camID>=0)
	{
		videoInput->init(configManager.camID);
	}
	else
	{
		videoInput->init(configManager.camSourceFilename.data());
	}

	if (configManager.showImage)
	{
		namedWindow(imageWindowName, CV_WINDOW_AUTOSIZE);
	}

	// Init matrices
	frameCaptured = new Mat(480,640,CV_8UC4);	// Warning: this assumes the resolution and 4 channel color depth

	// Initialize socket communication and server socket
	initServerSocket(configManager.serverPort);
}

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
	// TODO wait for desired timestamp if defined
	if (msg->desiredtimestamp > 0)
	{
		double freq = cv::getTickFrequency();
		double multiplierForUs = 1000000.0 / freq;
		// TODO: if desiredtimestamp is far away, sleep this thread...
		while( cv::getTickCount()*multiplierForUs < msg->desiredtimestamp );
	}

	// Taking a picture
	timeMeasurement.start(CamClient::TimeMeasurementCodeDefs::Capture);
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
	timeMeasurement.start(CamClient::TimeMeasurementCodeDefs::Answering);
	// TODO: timestamp and filesize
	long long timestamp = 0;
	long filesize = buff.size();
	std::ostringstream out;
	out << "{ \"type\":\"jpeg\", \"timestamp\": \"" << timestamp << "\", \"size\": \"" << filesize << "\" }#";

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
	timeMeasurement.finish(CamClient::TimeMeasurementCodeDefs::Answering);

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

void handleJSON(char *json, SOCKET sock)
{
	JsonMessage *msg = JsonMessage::parse(json);

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
		handleTakePicture((TakePictureMessage*)msg,sock);
		break;
	default:
		Logger::getInstance()->Log(Logger::LOGLEVEL_WARNING,"CamClient","Unknown message type!\n");
	}

	Logger::getInstance()->Log(Logger::LOGLEVEL_INFO,"CamClient","Command handled\n");
	return;
}



int main(int argc, char *argv[])
{
	if (argc>=2)
	{
		// INI file is given as command line parameter
		inifilename = argv[1];
	}
	init(inifilename);

	// TODO: measure times
	// TODO: allow using continuous open socket
	// TODO: allow shutting down or otherwise save the time measurement results into configManager.resultFileName
	// TODO: allow automatic camera parameter setting

	// TODO: Register the node

	openServerSocket();

	// Enter main loop: wait for commands and execute them.
	while(running)
	{
		// Wait for connection
		Logger::getInstance()->Log(Logger::LOGLEVEL_INFO,"CamClient","Waiting for connection\n");
		struct sockaddr_in addr;
		SOCKET sock = accept(serversock, (struct sockaddr *) &addr, NULL);
		if (sock < 0)
		{
			Logger::getInstance()->Log(Logger::LOGLEVEL_ERROR,"CamClient","Error on accept()\n");
			closeServerSocket();
			exit(3);
		}
		// Convert IP address to string
		char ipAddressStr[INET_ADDRSTRLEN];
		inet_ntop( AF_INET, &addr.sin_addr, ipAddressStr, INET_ADDRSTRLEN );
		Logger::getInstance()->Log(Logger::LOGLEVEL_INFO,"CamClient","Connection received from %s\n",ipAddressStr);

		// Handle connection
		char buffer[4096];	// Data receive buffer
		memset(buffer,0,4096);

		// TODO: wait for the whole message
		int n = recv(sock,buffer,4096,0);
		if (n < 0)
		{
			Logger::getInstance()->Log(Logger::LOGLEVEL_ERROR,"CamClient","Error on reading from socket.\n");
		}
		printf("Here is the message: %s\n",buffer);

		// Find begin and end of JSON: first '{' and last '}' before first '\0'
		char *start = strstr(buffer,"{");
		char *finish = strstr(buffer,"}");
		char *p;
		while((p = strstr(finish+1,"}")) != NULL)
		{
			finish = p;
		}
		char json[4096];
		memset(json,0,4096);
		memcpy(json,start,finish-start+1);
		printf("JSON: %s\n",json);

		// ---------- Message received, now handle it
		handleJSON(json, sock);

		// Close connection
		closesocket(sock); 
		Logger::getInstance()->Log(Logger::LOGLEVEL_INFO,"CamClient","Connection closed\n");
	}

	closeServerSocket();

	ofstream results;
	results.open(configManager.resultFileName.c_str(),ios_base::out);
	Logger::getInstance()->Log(Logger::LOGLEVEL_INFO,"CamClient","Results written to %s\n",configManager.resultFileName.c_str());
	results << "--- Main loop time measurement results:" << endl;
	timeMeasurement.showresults(&results);

	results << "--- Further details:" << endl;
	results << "max fps: " << timeMeasurement.getmaxfps(CamClient::TimeMeasurementCodeDefs::FrameAll) << endl;
	results << "Number of captured images: " << imageNumber << endl;

	results.flush();
	results.close();


	return 0;
}
