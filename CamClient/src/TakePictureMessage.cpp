#include <sstream>
#include "picojson.h"
#include "TakePictureMessage.h"
#include "Logger.h"

bool TakePictureMessage::parse(char *json)
{
	// find desiredtimestamp
	char *typePtr = strstr(json,"\"desiredtimestamp\":");
	if (!typePtr) return NULL;
	char *beginPtr = strstr(typePtr+18,"\"") + 1;
	if (!beginPtr) return NULL;
	char *endPtr = strstr(beginPtr,"\"");
	if (!endPtr) return NULL;
	char timestampString[100];
	memset(timestampString,0,100);
	strncpy(timestampString,beginPtr,endPtr-beginPtr);

	desiredtimestamp = _atoi64(timestampString);
		
	return true;
}

void TakePictureMessage::log()
{
	LogConfigTime::Logger::getInstance()->Log(LogConfigTime::Logger::LOGLEVEL_INFO,"Message","TakePictureMessage( desiredtimestamp=%Ld )",desiredtimestamp);
}
