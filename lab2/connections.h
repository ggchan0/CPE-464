#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include "linkedlist.h"

#define BACKLOG 128
#define MAXBUF 1024

#define PDU_LEN_SIZE 2
#define PDU_POS 0
#define FLAG_SIZE 1
#define FLAG_POS 2
#define HANDLE_LEN_SIZE 1
#define HANDLE_POS 3

#define INIT_PACKET 1
#define GOOD_HANDLE 2
#define BAD_HANDLE 3
#define BROADCAST 4
#define C_TO_C 5
#define ERR_PACKET 7
#define C_EXIT 8
#define C_EXIT_ACK 9
#define HANDLE_REQ 10
#define NUM_HANDLE_RES 11
#define HANDLE_RES 12
#define HANDLE_REQ_FIN 13

int setupClient(char * serverName, int port);
void sendPacket(int socketNum, char * sendBuf, int len);
void createInitPacket(char * buf, char * clientHandle, uint16_t * len);
void sendInitPacketToServer(int socketNum, char * clientHandle);

int setupServer(int port);
int acceptClient(int serverSocket);
int duplicateHandle(char * handle, Nodelist * list);
void sendClientInitErrorPacket(int clientSocket);
void handleClientInit(int clientSocket, Nodelist * list);
