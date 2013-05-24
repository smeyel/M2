#ifndef __MYCONFIGMANAGER_H
#define __MYCONFIGMANAGER_H
#include "stdlib.h"
#include "SimpleIniConfigReader.h"

using namespace LogConfigTime;

class MyConfigManager
{
	// This method is called by init of the base class to read the configuration values.
	virtual bool readConfiguration(char *filename)
	{
		SimpleIniConfigReader *SIreader = new SimpleIniConfigReader(filename);
		ConfigReader *reader = SIreader;

		showImage = reader->getBoolValue("main","showImage");
		serverPort = reader->getIntValue("main","serverPort");
		camSourceFilename = reader->getBoolValue("main","camSourceFilename");
		logFileName = reader->getStringValue("main","logFileName");
		usePs3eye = reader->getBoolValue("main","usePs3eye");
		sendMatImage = reader->getBoolValue("main","sendMatImage");
		camID = reader->getIntValue("main","camID");

		return true;
	}

public:
	void init(char *filename)
	{
		readConfiguration(filename);
	}

	// --- Settings
	bool showImage;
	bool sendMatImage;
	bool usePs3eye;
	int camID;	// ID of camera, of <0 for filename
	std::string camSourceFilename;	// If !usePs3eye, may be filename
	std::string logFileName;
	int serverPort;
};

#endif

