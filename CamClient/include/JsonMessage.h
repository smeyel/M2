#ifndef __JSONMESSAGE_H
#define __JSONMESSAGE_H

#define MAXTYPENAMELENGTH 100

#define DEBUG_JSON_IF_UNKNOWN

class JsonMessage
{
public:
	static JsonMessage *parse(char *json);

	static void DebugJson(char *json);

	virtual void log();
};

#endif
