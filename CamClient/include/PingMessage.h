#ifndef __PINGMESSAGE_H
#define __PINGMESSAGE_H

#include "JsonMessage.h"

class PingMessage : public JsonMessage
{
public:
	PingMessage(char *json);
	bool parse(char *json);
	virtual void log();
};

#endif
