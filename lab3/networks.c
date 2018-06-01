
// Hugh Smith April 2017
// Network code to support TCP/UDP client and server connections
// Code taken from Stop and Wait design and modified for Selective Reject

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
#include "cpe464.h"

int safeSend(uint8_t *packet, uint32_t len, Connection *connection) {
	int send_len = 0;

	if ((send_len = sendtoErr(connection->sk_num, packet, len, 0, (struct sockaddr *) &(connection->remote), sizeof(struct sockaddr_in6))) < 0) {
		perror("in send_buf(), sendto() call");
		exit(-1);
	}

	return send_len;
}

int safeRecv(int sk_num, char *data_buf, int len, Connection *connection) {
	int recv_len = 0;
	uint32_t remote_len = sizeof(struct sockaddr_in6);

	if ((recv_len = recvfrom(sk_num, data_buf, len, 0, (struct sockaddr *) &(connection->remote), &remote_len)) < 0) {
		perror("recv_buf, recvfrom");
		exit(-1);
	}

	connection->len = remote_len;

	return recv_len;
}

int sendBuf(uint8_t *buf, uint32_t len, Connection *connection, uint8_t flag, uint32_t seq_num, uint8_t * packet) {
	uint32_t sentLen = 0;
	uint32_t sendingLen = 0;

	if (len > 0) {
		memcpy(packet + HEADER_SIZE, buf, len);
	}

	sendingLen = createHeader(len, flag, seq_num, packet);

	sentLen = safeSend(packet, sendingLen, connection);

	return sentLen;
}

int recv_buf(uint8_t *buf, int len, int sk_num, Connection *connection, uint8_t *flag, int *seq_num) {
	char data_buf[MAX_LEN];
	int recv_len = 0;
	int dataLen = 0;

	recv_len = safeRecv(sk_num, data_buf, len, connection);

	dataLen = retrieveHeader(data_buf, recv_len, flag, seq_num);

	if (dataLen > 0) {
		memcpy(buf, data_buf + HEADER_SIZE, recv_len);
	}

	return dataLen;
}

int createHeader(uint32_t len, uint8_t flag, uint32_t seq_num, uint8_t *packet) {
	Header *aHeader = (Header *) packet;
	uint16_t checksum = 0;

	seq_num = htonl(seq_num);
	memcpy(&(aHeader->seq_num), &seq_num, sizeof(seq_num));

	aHeader->flag = flag;

	memset(&(aHeader->checksum), 0, sizeof(checksum));
	checksum = in_cksum((unsigned short *) packet, len + sizeof(Header));
	memcpy(&(aHeader->checksum), &checksum, sizeof(checksum));

	return len + HEADER_SIZE;
}

int retrieveHeader(char *data_buf, int recv_len, uint8_t *flag, uint32_t *seq_num) {
	Header *aHeader = (Header *) data_buf;
	int returnValue = 0;

	if (in_cksum((unsigned short*) data_buf, recv_len) != 0) {
		printf("CRC Error\n");
		returnValue = CRC_ERROR;
	} else {
		*flag = aHeader->flag;
		memcpy(seq_num, &(aHeader->seq_num), sizeof(aHeader->seq_num));
		*seq_num = ntohl(*seq_num);

		returnValue = recv_len - sizeof(Header);
	}

	return returnValue;
}

void insertIntoWindow(Window *window, uint8_t *packet, int packetLen, int seq_num) {
	int index = 0;

	index = seq_num % window->size;

	memcpy(window->buf[index].buffer, packet, packetLen);
	window->buf[index].buf_size = packetLen;
	window->buf[index].seq_num = seq_num;
	window->isValid[index] = 1;
}

void loadFromWindow(Window *window, uint8_t *packet, uint32_t *len_read, int seq_num) {
	int index = seq_num % window->size;

	memcpy(packet, window->buf[index].buffer, window->buf[index].buf_size);
	*len_read = window->buf[index].buf_size;
}

void removeFromWindow(Window *window, uint8_t *packet, int seq_num) {
	int index = seq_num % window->size;
	loadFromWindow(window, packet, seq_num);
	window->isValid[index] = 0;
}

void slideWindow(Window *window, int new_bottom) {
	int i, index;

	for (i = window->bottom; i < new_bottom; i++) {
		index = i % window->size;
		window->isValid[index] = 0;
	}

	window->bottom = new_bottom;
	if (window->bottom > window->middle) {
		window->middle = window->bottom;
	}
}

void freeWindow(Window *window) {
	int i;
	for (i = 0; i < window->size; i++) {
		windowBuf *b = window->buf[i];
		free(b->buffer);
		free(b);
	}
	free(window->buf);
	free(window->isValid);
}

void initWindow(Window *window, int buf_size, int window_size) {
	int i;
	window->bottom = 1;
	window->top = window_size;
	window->middle = 1;
	window->size = window_size;
	window->isValid = checked_calloc(sizeof(uint8_t) * window_size);
	window->buf = checked_calloc(sizeof(windowBuf *) * window_size);
	for (i = 0; i < window_size; i++) {
		(window->buf)[i] = checked_calloc(sizeof(windowBuf));
		(window->buf)[i].buffer = checked_calloc(sizeof(uint8_t) * buf_size  + HEADER_SIZE);
	}
}

void *checked_calloc(size_t size) {
	void *mem = NULL;
	if ((mem = calloc(size, sizeof(uint8_t))) == NULL) {
		perror("Calloc error:");
		exit(-1);
	}

	return mem;
}

/*
	Returns:
	doneState if calling this exceeds MAX_TRIES
	selectTimeoutState if the select times out
	dataReadyState if select returns indicating that data is ready for read
*/
int processSelect(Connection *client, int *retryCount, int selectTimeoutState, int dataReadyState, int doneState) {
	int returnValue = dataReadyState;

	(*retryCount)++;
	if (*retryCount > MAX_TRIES) {
		printf("Sent data %d times, no ACK, client is probably gone\n", MAX_TRIES);
		returnValue = doneState;
	} else {
		if (select_call(client->sk_num, SHORT_TIME, 0, NOT_NULL) == 1) {
			*retryCount = 0;
			returnValue = dataReadyState;
		} else {
			returnValue = selectTimeoutState;
		}
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
	if ((socketNum = socket(AF_INET6, SOCK_DGRAM, 0)) < 0)
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

int udpClientSetup(char * hostName, int portNumber, Connection *connection) {
	// currently only setup for IPv4
	char ipString[INET6_ADDRSTRLEN];
	uint8_t * ipAddress = NULL;
	connection->sk_num = 0;
	connection->len = sizeof(struct sockaddr_in);

	// create the socket
	if ((connection->sk_num = socket(AF_INET6, SOCK_DGRAM, 0)) < 0)
	{
		perror("udpClientSetup, socket");
		exit(-1);
	}

	if ((ipAddress = gethostbyname6(hostName, &(connection->remote))) == NULL)
	{
		exit(-1);
	}

	connection->remote.sin6_port = ntohs(portNumber);
	connection->remote.sin6_family = AF_INET6;

	inet_ntop(AF_INET6, ipAddress, ipString, sizeof(ipString));
	printf("Server info - IP: %s Port: %d \n", ipString, portNumber);

	return connection->sk_num;
}

int select_call(int sk_num, int sec, int microsec, int set_null) {
	fd_set fdvar;
	struct timeval aTimeout;
	struct timeval *timeout = NULL;

	if (set_null == NOT_NULL) {
		aTimeout.tv_sec = sec;
		aTimeout.tv_usec = microsec;
		timeout = &aTimeout;
	}

	FD_ZERO(&fdvar);
	FD_SET(sk_num, &fdvar);

	if (select(sk_num + 1, (fd_set *) &fdvar, (fd_set *) 0, (fd_set *) 0, timeout) < 0) {
		perror("select");
		exit(-1);
	}

	if (FD_ISSET(sk_num, &fdvar)) {
		return 1;
	} else {
		return 0;
	}
}
