#ifndef __MYCONFIGMANAGER_H
#define __MYCONFIGMANAGER_H
#include "stdlib.h"
#include "SimpleIniConfigReader.h"

using namespace LogConfigTime;

class MyConfigManager
{
	virtual bool readConfiguration(char *filename)
	{
		SimpleIniConfigReader *SIreader = new SimpleIniConfigReader(filename);
		ConfigReader *reader = SIreader;

		logFileName = reader->getStringValue("main","logFileName");
		phoneIpAddress = reader->getStringValue("main","phoneIpAddress");
		phonePort = reader->getIntValue("main","phonePort");

		return true;
	}

public:
	void init(char *filename)
	{
		readConfiguration(filename);
	}

	// --- Settings
	std::string logFileName;
	std::string phoneIpAddress;
	int phonePort;
};

#endif

