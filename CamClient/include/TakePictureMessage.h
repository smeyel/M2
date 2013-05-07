#ifndef __TAKEPICTUREMESSAGE_H
#define __TAKEPICTUREMESSAGE_H

#include "JsonMessage.h"

class TakePictureMessage : public JsonMessage
{
public:
	long long desiredtimestamp;

	TakePictureMessage(char *json)
	{
		parse(json);
	}

	bool parse(char *json);
	virtual void log();
};

#endif