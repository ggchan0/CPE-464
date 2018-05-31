
// Hugh Smith April 2017
// Network code to support TCP/UDP client and server connections

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/uio.h>
#include <sys/time.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <strings.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#include "networks.h"
#include "gethostbyname.h"

int safeRecvfrom(int socketNum, void * buf, int len, int flags, struct sockaddr *srcAddr, int * addrLen)
{
	int returnValue = 0;
	if ((returnValue = recvfrom(socketNum, buf, (size_t) len, flags, srcAddr, (socklen_t *) addrLen)) < 0)
	{
		perror("recvfrom: ");
		exit(-1);
	}

	return returnValue;
}

int safeSendto(int socketNum, void * buf, int len, int flags, struct sockaddr *srcAddr, int addrLen)
{
	int returnValue = 0;
	if ((returnValue = sendto(socketNum, buf, (size_t) len, flags, srcAddr, (socklen_t) addrLen)) < 0)
	{
		perror("sendto: ");
		exit(-1);
	}

	return returnValue;
}

int safeRecv(int socketNum, void * buf, int len, int flags)
{
	int returnValue = 0;
	if ((returnValue = recv(socketNum, buf, (size_t) len, flags)) < 0)
	{
		perror("recv: ");
		exit(-1);
	}

	return returnValue;
}

int safeSend(int socketNum, void * buf, int len, int flags)
{
	int returnValue = 0;
	if ((returnValue = send(socketNum, buf, (size_t) len, flags)) < 0)
	{
		perror("send: ");
		exit(-1);
	}

	return returnValue;
}


// This function sets the server socket. The function returns the server
// socket number and prints the port number to the screen.

int udpServerSetup(int portNumber)
{
	struct sockaddr_in6 server;
	int socketNum = 0;
	int serverAddrLen = 0;

	// create the socket
	if ((socketNum = socket(AF_INET6,SOCK_DGRAM,0)) < 0)
	{
		perror("socket() call error");
		exit(-1);
	}

	// set up the socket
	server.sin6_family = AF_INET6;    		// internet (IPv6 or IPv4) family
	server.sin6_addr = in6addr_any ;  		// use any local IP address
	server.sin6_port = htons(portNumber);   // if 0 = os picks

	// bind the name (address) to a port
	if (bind(socketNum,(struct sockaddr *) &server, sizeof(server)) < 0)
	{
		perror("bind() call error");
		exit(-1);
	}

	/* Get the port number */
	serverAddrLen = sizeof(server);
	getsockname(socketNum,(struct sockaddr *) &server,  &serverAddrLen);
	printf("Server using Port #: %d\n", ntohs(server.sin6_port));

	return socketNum;

}

int udpClientSetup(char *hostname, int portNumber, Connection *connection) {
	struct hostent *hp = NULL;

	connection->sk_num = 0;
	connection->len = sizeof(struct sockaddr_in);

	if ((connection->sk_num = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		perror("udpClientSetup, socket");
		exit(-1);
	}

	connection->remote.sin_family = AF_INET;

	hp = gethostbyname(hostname);

	if (hp == NULL) {
		printf("Host not found: %s\n", hostname);
		return -1;
	}

	memcpy(&(connection->remote.sin_addr), hp->h_addr, hp->h_length);

	connection->remote.sin_port = htons(portNumber);

	return 0;
}

int setupUdpClientToServer(struct sockaddr_in6 *server, char * hostName, int portNumber)
{
	// currently only setup for IPv4
	int socketNum = 0;
	char ipString[INET6_ADDRSTRLEN];
	uint8_t * ipAddress = NULL;

	// create the socket
	if ((socketNum = socket(AF_INET6, SOCK_DGRAM, 0)) < 0)
	{
		perror("socket() call error");
		exit(-1);
	}

	if ((ipAddress = gethostbyname6(hostName, server)) == NULL)
	{
		exit(-1);
	}

	server->sin6_port = ntohs(portNumber);
	server->sin6_family = AF_INET6;

	inet_ntop(AF_INET6, ipAddress, ipString, sizeof(ipString));
	printf("Server info - IP: %s Port: %d \n", ipString, portNumber);

	return socketNum;
}
