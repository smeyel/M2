#include <iostream>
#include <fstream>
using namespace std;

#include <stdio.h>
#include <conio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <winsock2.h>
#include <ws2tcpip.h>

#include <winsock.h>
#include <io.h>

#include <windows.h>

#define RCVBUFSIZE 8192

static void error_exit(char *errorMessage) {

    fprintf(stderr,"%s: %d\n", errorMessage, WSAGetLastError());
    exit(EXIT_FAILURE);
}




// ----------------------------------------------------------------------------

class PhoneProxy
{
private:
	SOCKET sock;

public:
	void Connect(char *ip, int port);
	void Disconnect();

	void RequestPhoto(long long desiredTimeStamp);
	void RequestPing();
	void Receive(char *filename);	// For PONG, filename has no effect.
	void ReceiveDebug();
	void RequestLog();

private:
	void ProcessIncomingJSON(int sock,char *buffer, char *filename);
};

void PhoneProxy::RequestPhoto(long long desiredTimeStamp)
{
    char buffer[100];
    int len;
	sprintf(buffer,"{ \"type\": \"takepicture\", \"desiredtimestamp\": \"%lld\" }#",desiredTimeStamp);
    len = strlen(buffer);

    if (send(sock, buffer, len, 0) != len)
        error_exit("send() has sent a different number of bytes than excepted !!!!");
	/*int iResult = shutdown(sock, SD_SEND);
	if (iResult == SOCKET_ERROR) {
        printf("shutdown failed with error: %d\n", WSAGetLastError());
		Disconnect();
        return;
    }*/
	return;
}

void PhoneProxy::RequestPing()
{
	char *cmd = "{ \"type\": \"ping\", \"desiredtimestamp\": \"0\" }#";
    int len;
    len = strlen(cmd);

    if (send(sock, cmd, len, 0) != len)
        error_exit("send() has sent a different number of bytes than expected !!!!");
	/*int iResult = shutdown(sock, SD_SEND);
	if (iResult == SOCKET_ERROR) {
        printf("shutdown failed with error: %d\n", WSAGetLastError());
		Disconnect();
        return;
    }*/
	return;
}

void PhoneProxy::RequestLog()
{
	char *cmd = "{ \"type\": \"sendlog\", \"desiredtimestamp\": \"0\" }#";
    int len;
    len = strlen(cmd);

    if (send(sock, cmd, len, 0) != len)
        error_exit("send() has sent a different number of bytes than expected !!!!");
	/*int iResult = shutdown(sock, SD_SEND);
	if (iResult == SOCKET_ERROR) {
        printf("shutdown failed with error: %d\n", WSAGetLastError());
		Disconnect();
        return;
    }*/
	return;
}

void PhoneProxy::Receive(char *filename)
{
	// Receive response
	int totalBytes = 0;
	char buffer[RCVBUFSIZE] = "";
	int received = 0;

	// Read JSON part (1 byte at once)
	char c;
	char *bufPtr = buffer;
	*bufPtr = 0;
	//while ((received = recv(sock, buffer, RCVBUFSIZE, 0)) > 0);
	while ((received = recv(sock, &c, 1, 0)) > 0) 
	{
		if (c != '#')	// Not at end of JSON
		{
			*bufPtr = c;
			bufPtr++;
			*bufPtr = 0;
		}
		else
		{
			if (bufPtr-buffer>2)	// Do not stop for UTF-8 initial 2 bytes
				break;
		}
	}
	ProcessIncomingJSON(sock,buffer+1,filename);
}

void PhoneProxy::ReceiveDebug()
{
	// Receive response
	char buffer[RCVBUFSIZE] = "";
	int received = recv(sock, buffer, RCVBUFSIZE, 0);
	cout << "DEBUG RECEIVE:" << endl << buffer << endl << "END RECEIVE" << endl;
	return;
}



void PhoneProxy::Connect(char *ip, int port)
{
	struct sockaddr_in server;
    struct hostent *host_info;
    unsigned long addr;
	int iResult;

	cout << "Connecting..." << endl;

    WORD wVersionRequested;
    WSADATA wsaData;
    wVersionRequested = MAKEWORD (1, 1);
    if (WSAStartup (wVersionRequested, &wsaData) != 0)
        error_exit( "Initialisation of Winsock failed");
    else
        printf("Winsock Initialised\n");

    sock = socket( AF_INET, SOCK_STREAM, 0 );

    if (sock < 0)
        error_exit( "Socket error");

    memset( &server, 0, sizeof (server));
    if ((addr = inet_addr( ip)) != INADDR_NONE) {
        memcpy( (char *)&server.sin_addr, &addr, sizeof(addr));
    }
    else {
        host_info = gethostbyname(ip);
        if (NULL == host_info)
            error_exit("Unknown Server");
        memcpy( (char *)&server.sin_addr,
                host_info->h_addr, host_info->h_length );
    }

    server.sin_family = AF_INET;
    server.sin_port = htons( port );

    if(connect(sock,(struct sockaddr*)&server,sizeof(server)) <0)
        error_exit("Connection to the server failed");
}

void PhoneProxy::Disconnect()
{
	closesocket(sock);
	WSACleanup();
	sock = -1;
}

void PhoneProxy::ProcessIncomingJSON(int sock,char *buffer, char *filename)
{
	cout << "JSON received:" << endl << buffer << endl << "End of JSON" << endl;

	// Parse JSON
	char *posJPEG = strstr(buffer,"JPEG");
	char *posTimeStamp = strstr(buffer,"timestamp");
	char *posPong = strstr(buffer,"pong");
	char *posLog = strstr(buffer, "measurementlog");

	if (posPong)
	{
		cout << "PONG received, nothing left to do..." << endl;
	}
	else if (posJPEG)
	{
		char tmpS[100];
		char *posTimeStampValue = posTimeStamp + 12;
		char *posTimeStampValueEnd = strstr(posTimeStampValue,"\"");
		int len = posTimeStampValueEnd-posTimeStampValue;
		strncpy(tmpS, posTimeStampValue, len );
		*(tmpS+len) = 0;
		int timestamp = atoi(tmpS);

		char *posSize = strstr(buffer,"size");
		char *posSizeValue = posSize + 7;
		char *posSizeValueEnd = strstr(posSizeValue,"\"");
		len = posSizeValueEnd-posSizeValue;
		strncpy(tmpS, posSizeValue, len );
		*(tmpS+len) = 0;
		int jpegSize = atoi(tmpS);

		cout << "Receiving JPEG. Timestamp=" << timestamp << ", size=" << jpegSize << endl;
	
		char receiveBuffer[RCVBUFSIZE];
		int received;
		int jpegBytes = 0;

		ofstream outFile;
		if (outFile != NULL) 
		{
			outFile.open(filename , ofstream::binary);
			cout << "File opened!" << endl;
		} else 
		{
			cout << "Can't open file!" << endl;
		}

		//while ((received = recv(sock, receiveBuffer, RCVBUFSIZE, 0)) > 0)
		while (jpegBytes != jpegSize)
		{
			received = recv(sock, receiveBuffer, RCVBUFSIZE, 0);
			jpegBytes += received;
			if (outFile.is_open()) 
			{
				outFile.write(receiveBuffer, received); 
				cout << " (Total: " << jpegBytes << " B)" << endl;
			}
			else
			{
				cout << "Error in recv() function, received bytes = " << received << endl;
			}
		}

		outFile.flush();
		outFile.close();

		cout << "JPEG received successfully. Sent size=" << jpegSize << ", received size=" << jpegBytes << endl;
	} else if(posLog)
	{
		ofstream file;
		file.open(filename);
		file << (buffer+1); //+1 because of 1 caracter at the beginning
		file.close();
		//cout << "Received message:"<< buffer << endl;
	}

}


int main( int argc, char *argv[])
{
	char *ip = "192.168.245.101";
	int port = 6000;
	PhoneProxy proxy;

	char tmpBuff[100];
/*	cout << "Press enter to start connecting..." << endl;
	cin >> tmpBuff;*/

/*	proxy.Connect(ip,port);
	proxy.RequestPing();
	proxy.Receive("d:\\temp\\nothing.txt");
	proxy.Disconnect();*/

	proxy.Connect(ip,port);
	/*proxy.RequestPhoto(0);
	proxy.Receive("d:\\temp\\image1.jpg");
	//proxy.ReceiveDebug();
	//proxy.Disconnect();
	//proxy.Connect(ip,port);
	proxy.RequestPhoto(0);
	proxy.Receive("d:\\temp\\image2.jpg");
	proxy.RequestPhoto(0);
	proxy.Receive("d:\\temp\\image3.jpg");*/

	//proxy.RequestPing();
	//proxy.Receive("d:\\temp\\nothing.txt");

	for(int i=0; i<=5;i++)
	{
		Sleep(1500);
		sprintf(tmpBuff,"d:\\temp\\image%d.jpg",i);
		proxy.RequestPhoto(0);
		proxy.Receive(tmpBuff);
	}
	//proxy.RequestPhoto(8151784932);
	//proxy.Receive(tmpBuff);
	//proxy.RequestPhoto(0);
	//proxy.Receive(tmpBuff);
	proxy.RequestLog();
	proxy.Receive("d:\\temp\\message.txt");
	proxy.Disconnect();
	cout << "Press enter to finish..." << endl;
	cin >> tmpBuff;


	return EXIT_SUCCESS;
}
