#ifndef __MYCONFIGMANAGER_H
#define __MYCONFIGMANAGER_H
#include "stdlib.h"
#include "SimpleIniConfigReader.h"

using namespace LogConfigTime;

class MyConfigManager
{
    virtual bool readConfiguration(const char *filename)
	{
		SimpleIniConfigReader *SIreader = new SimpleIniConfigReader(filename);
		ConfigReader *reader = SIreader;

		logFileName = reader->getStringValue("main","logFileName");
		phoneIpAddress = reader->getStringValue("main","phoneIpAddress");
		phonePort = reader->getIntValue("main","phonePort");
		useDesiredTimestamp = reader->getBoolValue("main","useDesiredTimestamp");
		remoteMLogFilename = reader->getStringValue("main","remoteMLogFilename");
		localMLogFilename = reader->getStringValue("main","localMLogFilename");
		MLogFilename = reader->getStringValue("main","MLogFilename");
		showImage = reader->getBoolValue("main","showImage");
		camIntrinsicParamsFileName = reader->getStringValue("main","camIntrinsicParamsFileName");
		return true;
	}

public:
    void init(const char *filename)
	{
		readConfiguration(filename);
	}

	// --- Settings
	bool showImage;
	bool useDesiredTimestamp;
	std::string logFileName;
	std::string phoneIpAddress;
	int phonePort;
	std::string localMLogFilename;
	std::string remoteMLogFilename;
	std::string MLogFilename;
	std::string camIntrinsicParamsFileName;
};

#endif
