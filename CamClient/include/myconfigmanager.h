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

		showInputImage = reader->getBoolValue("main","showInputImage");
		serverPort = reader->getIntValue("main","serverPort");
		camSource = reader->getBoolValue("main","camSource");
		logFileName = reader->getStringValue("main","logFileName");

		return true;
	}

public:
	void init(char *filename)
	{
		readConfiguration(filename);
	}

	// --- Settings
	bool showInputImage;
	std::string camSource;
	std::string logFileName;
	int serverPort;
};

#endif

