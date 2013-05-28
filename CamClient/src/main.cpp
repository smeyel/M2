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
#include "PhoneServer.h"

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

PhoneServer server;

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

	// Set auto gain, exposure and white balance
	videoInput->SetNormalizedGain(-1);
	videoInput->SetNormalizedExposure(-1);
	videoInput->SetNormalizedWhiteBalance(-1,-1,-1);

	if (configManager.showImage)
	{
		namedWindow(imageWindowName, CV_WINDOW_AUTOSIZE);
	}

	// Init matrices
	frameCaptured = new Mat(480,640,CV_8UC4);	// Warning: this assumes the resolution and 4 channel color depth

	// Initialize socket communication and server socket
	server.InitServer(configManager.serverPort);
}


int main(int argc, char *argv[])
{
	if (argc>=2)
	{
		// INI file is given as command line parameter
		inifilename = argv[1];
	}
	init(inifilename);

	if (server.RegisterNode("avalon.aut.bme.hu","~kristof/smeyel/smeyel_reg.php?IP=127.0.0.1:6000"))	// TODO: do not hardwire the port!
	{
		cout << "Error: could not register the node..." << endl;
	}

	server.ListenServerSocket();

	// Enter main loop: wait for commands and execute them.
	while(running)
	{
		// Wait for connection
		Logger::getInstance()->Log(Logger::LOGLEVEL_INFO,"CamClient","Waiting for connection\n");
		cout << "Waiting for connection." << endl;
		struct sockaddr_in addr;
		SOCKET sock = accept(server.GetServerSocket(), (struct sockaddr *) &addr, NULL);
		if (sock < 0 || sock == INVALID_SOCKET)
		{
//			WSAGetLastError
			Logger::getInstance()->Log(Logger::LOGLEVEL_ERROR,"CamClient","Error on accept(), trying to re-initialize server.\n");
			server.DisconnectServer();
			server.InitServer(configManager.serverPort);
			server.ListenServerSocket();
			cout << "Server rebinded, waiting for connection." << endl;
			sock = accept(server.GetServerSocket(), (struct sockaddr *) &addr, NULL);
			if (sock < 0 || sock == INVALID_SOCKET)
			{
				Logger::getInstance()->Log(Logger::LOGLEVEL_ERROR,"CamClient","Error on accept(), 2nd try failed, exiting.\n");
				server.DisconnectServer();
				exit(1);
			}
		}
		// Convert IP address to string
		char ipAddressStr[INET_ADDRSTRLEN];
		inet_ntop( AF_INET, &addr.sin_addr, ipAddressStr, INET_ADDRSTRLEN );
		Logger::getInstance()->Log(Logger::LOGLEVEL_INFO,"CamClient","Connection received from %s\n",ipAddressStr);
		cout << "Connected." << endl;

		// TODO: while socket is not closed by remote size, repeat waiting for commands and execute them...
		bool connectionOpen = true;
		server.SetSock(sock);
		while(connectionOpen)
		{
			// Handle connection

			// TODO: wait for the whole message
			timeMeasurement.start(CamClient::TimeMeasurementCodeDefs::ReceiveCommand);
			JsonMessage *msg = server.ReceiveNew();
			timeMeasurement.finish(CamClient::TimeMeasurementCodeDefs::ReceiveCommand);
			
			if (!msg)
			{
				// The connection was closed from the remote side.
				// (Or a malformed JSON was received...)
				Logger::getInstance()->Log(Logger::LOGLEVEL_ERROR,"CamClient","Error on reading from socket. Closing connection.\n");
				connectionOpen=false;
				break;
			}

			// ---------- Message received, now handle it
			timeMeasurement.start(CamClient::TimeMeasurementCodeDefs::HandleJson);
			handleJSON(msg, &server);
			timeMeasurement.finish(CamClient::TimeMeasurementCodeDefs::HandleJson);
		}

		// Close connection
		server.Disconnect();
		Logger::getInstance()->Log(Logger::LOGLEVEL_INFO,"CamClient","Connection closed\n");
	}

	server.DisconnectServer();

	return 0;
}
