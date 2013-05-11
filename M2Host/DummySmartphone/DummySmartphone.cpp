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

#define PORT 6000
#define RCVBUFSIZE 8192

static void error_exit(char *errorMessage) {

    fprintf(stderr,"%s: %d\n", errorMessage, WSAGetLastError());
    exit(EXIT_FAILURE);
}

void ServiceRequest(int connSocket)
{
	char buffer[RCVBUFSIZE] = {0};
	char sendbuffer[RCVBUFSIZE] = {0};

	cout << "ServiceRequest started for opened connection..." << endl;

	int totalbytes=0;
	int received;
	while ((received = recv(connSocket, buffer, RCVBUFSIZE, 0)) > 0) 
	{
		cout << "R:" << received << " ";
		if (received > 0) 
		{
			totalbytes += received;
			cout << " (Total: " << totalbytes << " B)" << endl;
		}
		else
		{
			cout << "Error in recv() function, received bytes = " << received << endl;
		}
	}	

	*(buffer + totalbytes) = 0;
	cout << "Received line: [" << buffer << "]" << endl;

	if (strstr(buffer,"takepicture"))
	{
		cout << "Recognized requrest: take picture" << endl;
		sprintf(sendbuffer, "{ \"type\": \"JPEG\", \"size\": \"10\", \"timestamp\": \"123456\" }#1234567890");
	}
	else if (strstr(buffer,"ping"))
	{
		cout << "Recognized requrest: ping" << endl;
		sprintf(sendbuffer, "{ \"type\": \"pong\" }#");
	}
	else
	{
		cout << "Unknown request! :(" << endl;
		sprintf(sendbuffer, "{ \"type\": \"error\" }#");
	}

	//sprintf(buffer, "{ \"type\": \"pong\" }");
	char *ptr = strstr(sendbuffer,"#");
	int len = strlen(sendbuffer);
	*ptr=0;	// Changes string length!!!

    if (send(connSocket, sendbuffer, len, 0) != len)
        error_exit("send() has sent a different number of bytes than expected !!!!");

	cout << "ServiceRequest finished..." << endl;
}

int main( int argc, char *argv[]) {
	// Init socket
    WORD wVersionRequested;
    WSADATA wsaData;
    wVersionRequested = MAKEWORD (1, 1);
    if (WSAStartup (wVersionRequested, &wsaData) != 0)
        error_exit( "Initialisation of Winsock failed");
    else
        printf("Winsock Initialised\n");

	// Create server socket
    SOCKET sock;
    int    conn;
    struct sockaddr_in servaddr;
    
    /*  Create socket  */
    sock = socket( AF_INET, SOCK_STREAM, 0 );
    if (sock < 0)
        error_exit( "Socket error");

    memset( &servaddr, 0, sizeof (servaddr));
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons( PORT );

    /*  Assign socket address to socket  */ 
    if ( bind(sock, (struct sockaddr *) &servaddr, sizeof(servaddr)) < 0 )
		error_exit("Couldn't bind listening socket.");

    /*  Make socket a listening socket  */
    if ( listen(sock, 1024) < 0 )
		error_exit("Call to listen failed.");

    /*  Loop infinitely to accept and service connections  */
    while ( 1 ) {
		cout << "Listening..." << endl;
		/*  Wait for connection  */

		if ( (conn = accept(sock, NULL, NULL)) < 0 )
			error_exit("Error calling accept()");

		cout << "Incoming connection..." << endl;

/*		if ( close(listener) < 0 )
			error_exit("Error closing listening socket in child.");*/

		// Service the request
		ServiceRequest(conn);

		/*  Close connected socket and exit  */
		if ( closesocket(conn) < 0 )
			error_exit("Error closing connection socket.");
    }
	closesocket(sock);

	// Cleanup
	WSACleanup();
	return EXIT_SUCCESS;
}