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
#include "SendlogMessage.h"
#include "TakePictureMessage.h"

//#include "picojson.h"
#include "messagehandling.h"


using namespace cv;
using namespace std;
using namespace LogConfigTime;

char *inifilename = "default.ini";	// 1st command line parameter overrides it

const char *imageWindowName = "CamClient JPEG";

MyConfigManager configManager;
TimeMeasurement timeMeasurement;
Mat *frameCaptured;
VideoInput *videoInput;
int imageNumber=0;	// Counts the number of images taken
bool running=true;	// Used to finish the main loop...
SOCKET serversock;

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
	//logger->SetLogLevel(Logger::LOGLEVEL_INFO);
	logger->SetLogLevel(Logger::LOGLEVEL_ERROR);
	Logger::getInstance()->Log(Logger::LOGLEVEL_VERBOSE,"CamClient","CamClient started\n");
    time_t t = time(0);   // get time now
    struct tm * now = localtime( & t );
	Logger::getInstance()->Log(Logger::LOGLEVEL_INFO,"CamClient","Current time: %d-%d-%d, %d:%d:%d\n",
		(now->tm_year + 1900),(now->tm_mon + 1),now->tm_mday,now->tm_hour,now->tm_min,now->tm_sec );
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
		cout << "Waiting for connection." << endl;
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
		cout << "Connected." << endl;

		// TODO: while socket is not closed by remote size, repeat waiting for commands and execute them...
		bool connectionOpen = true;
		while(connectionOpen)
		{
			// Handle connection
			char buffer[4096];	// Data receive buffer
			memset(buffer,0,4096);

			// TODO: wait for the whole message
			timeMeasurement.start(CamClient::TimeMeasurementCodeDefs::ReceiveCommand);
			int n = 0;
			while (n<=0)
			{
				n = recv(sock,buffer,4096,0);
				if (n < 0)
				{
					Logger::getInstance()->Log(Logger::LOGLEVEL_ERROR,"CamClient","Error on reading from socket. Closing connection.\n");
					connectionOpen=false;
					break;
				}
			}
			if (!connectionOpen)
			{
				break;
			}
			//printf("Here is the message: %s\n",buffer);

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
			//printf("JSON: %s\n",json);
			timeMeasurement.finish(CamClient::TimeMeasurementCodeDefs::ReceiveCommand);

			// ---------- Message received, now handle it
			timeMeasurement.start(CamClient::TimeMeasurementCodeDefs::HandleJson);
			handleJSON(json, sock);
			timeMeasurement.finish(CamClient::TimeMeasurementCodeDefs::HandleJson);
		}

		// Close connection
		closesocket(sock); 
		Logger::getInstance()->Log(Logger::LOGLEVEL_INFO,"CamClient","Connection closed\n");
	}

	closeServerSocket();

	return 0;
}
