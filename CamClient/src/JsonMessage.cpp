#include <stdio.h>
#include <string.h>
#include "JsonMessage.h"
#include "PingMessage.h"
#include "TakePictureMessage.h"

#include "Logger.h"

// Used for debugging JSON messages
#include <sstream>
#include "picojson.h"

JsonMessage *JsonMessage::parse(char *json)
{
	// Search for type info
	char *typePtr = strstr(json,"\"type\":");
	if (!typePtr) return NULL;
	char *beginPtr = strstr(typePtr+6,"\"") + 1;
	if (!beginPtr) return NULL;
	char *endPtr = strstr(beginPtr,"\"");
	if (!endPtr) return NULL;
	char typeString[MAXTYPENAMELENGTH];
	memset(typeString,0,MAXTYPENAMELENGTH);
	strncpy(typeString,beginPtr,endPtr-beginPtr);
		
	// Let the corresponding class parse it
	if (!strcmp(typeString,"ping"))
	{
		return new PingMessage(json);
	}
	else if (!strcmp(typeString,"takepicture"))
	{
		return new TakePictureMessage(json);
	}

#ifdef DEBUG_JSON_IF_UNKNOWN
	DebugJson(json);
#endif

	return NULL;
}

void JsonMessage::DebugJson(char *json)
{
	std::istringstream jsonStream(json);

	picojson::value v;
  
	// read json value from stream
	jsonStream >> v;
	if (jsonStream.fail()) {
		std::cerr << picojson::get_last_error() << std::endl;
		return;
	}
  
	// dump json object
	std::cout << "---- dump input ----" << std::endl;
	std::cout << v << std::endl;

	// accessors
	std::cout << "---- analyzing input ----" << std::endl;
	if (v.is<picojson::null>()) {
		std::cout << "input is null" << std::endl;
	} else if (v.is<bool>()) {
		std::cout << "input is " << (v.get<bool>() ? "true" : "false") << std::endl;
	} else if (v.is<double>()) {
		std::cout << "input is " << v.get<double>() << std::endl;
	} else if (v.is<std::string>()) {
		std::cout << "input is " << v.get<std::string>() << std::endl;
	} else if (v.is<picojson::array>()) {
		std::cout << "input is an array" << std::endl;
		const picojson::array& a = v.get<picojson::array>();
		for (picojson::array::const_iterator i = a.begin(); i != a.end(); ++i)
		{
			std::cout << "  " << *i << std::endl;
		}
	}
	else if (v.is<picojson::object>()) {
		std::cout << "input is an object" << std::endl;
		const picojson::object& o = v.get<picojson::object>();
		for (picojson::object::const_iterator i = o.begin(); i != o.end(); ++i)
		{
			std::cout << i->first << "  " << i->second << std::endl;
		}
	}
	return;
}

void JsonMessage::log()
{
	LogConfigTime::Logger::getInstance()->Log(LogConfigTime::Logger::LOGLEVEL_INFO,"Message","PingMessage()");
}
