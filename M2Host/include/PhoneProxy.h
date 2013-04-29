#ifndef __PHONEPROXY_H
#define __PHONEPROXY_H
#include <fstream>
#include <winsock.h>

class PhoneProxy
{
private:
	SOCKET sock;

public:
	std::ofstream *log;
	_int64 lastReceivedTimeStamp;

	PhoneProxy(std::ofstream *aLog)
	{
		lastReceivedTimeStamp = 0;
		log = aLog;
	}

	void Connect(char *ip, int port);
	void Disconnect();

	void RequestPhoto(_int64 desiredTimeStamp);
	void RequestPing();
	void Receive(char *filename);	// For PONG, filename has no effect.
	void ReceiveDebug();

private:
	void ProcessIncomingJSON(int sock,char *buffer, char *filename);
};

#endif