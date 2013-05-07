#include <sstream>
#include "PingMessage.h"
#include "Logger.h"

PingMessage::PingMessage(char *json)
{
	parse(json);
}

bool PingMessage::parse(char *json)
{
	return true;
}

void PingMessage::log()
{
	LogConfigTime::Logger::getInstance()->Log(LogConfigTime::Logger::LOGLEVEL_INFO,"Message","PingMessage()");
}
