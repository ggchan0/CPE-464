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
#include <ctype.h>

#include "linkedlist.h"

#define BACKLOG 128
#define MAXBUF 2048
#define MAX_TEXT_SIZE 200
#define MAX_HANDLE_SIZE 100

#define PDU_LEN_SIZE 2
#define PDU_POS 0
#define FLAG_SIZE 1
#define FLAG_POS 2
#define HANDLE_LEN_SIZE 1
#define HANDLE_POS 3
#define HANDLE_LABEL_START 4
#define CHAT_HEADER_SIZE 3
#define HANDLE_RES_SIZE 4

#define INIT_PACKET 1
#define GOOD_HANDLE 2
#define BAD_HANDLE 3
#define BROADCAST_MESSAGE 4
#define C_TO_C 5
#define ERR_PACKET 7
#define C_EXIT 8
#define C_EXIT_ACK 9
#define HANDLE_REQ 10
#define NUM_HANDLE_RES 11
#define HANDLE_RES 12
#define HANDLE_REQ_FIN 13

void sendPacket(int socketNum, char * sendBuf, int len);
void readFromSocket(int socketNum, char * buf, int * messageLen);

char * doubleSize(char *str, int length);
char *readline(FILE *file);

int setupClient(char * serverName, int port);

void createInitPacket(char * buf, char * clientHandle, uint16_t * len);
void sendInitPacketToServer(int socketNum, char * clientHandle);
void recvConfirmationFromServer(int serverSocket, char * clientHandle);
void sendMessagePacket(int serverSocket, char * handle, char * destBuf, int destBufSize, char * buf, int bytesCopied);
void messageServer(int serverSocket, char * handle, char * destBuf, int destBufSize, char * text);
void parseAndSendMessage(int serverSocket, char * handle, char * buf);
void sendBroadcastPacket(int serverSocket, char * handle, char * buf, int bytesCopied);
void broadcastToServer(int serverSocket, char * handle, char * text);
void createListHandlesPacket(char * buf);
void sendListHandlesPacketToServer(int socketNum);
void createExitPacket(char * buf);
void sendExitPacketToServer(int socketNum);
void processInput(char * str, char * handle, int serverSocket);
void receiveBroadcastMessage(char * buf);
void receiveMessage(char * buf);
void receiveErrorPacket(char * buf);
void processHandle(int serverSocket);
void processHandleReqFin(int serverSocket);
void processListHandles(int serverSocket, char * buf);
void processMessage(int serverSocket);

void enterInteractiveMode(char * clientHandle, int serverSocket) ;

int setupServer(int port);
int acceptClient(int serverSocket);
void handleClientExit(int clientSocket, Nodelist * list);
int duplicateHandle(char * handle, Nodelist * list);
void sendClientInitErrorPacket(int clientSocket);
void sendClientInitSuccessPacket(int clientSocket);
void handleClientInit(int clientSocket, char * buf, Nodelist *list);
void handleInvalidHandle(int clientSocket, char * dest);
void handleMessage(int clientSocket, char * buf, Nodelist *list);
void handleBroadcast(int clientSocket, char * buf, Nodelist *list);
void sendClientExitAckPacket(int clientSocket);
void sendClientHandleResPacket(int clientSocket, Nodelist *list);
void sendClientHandlePacket(int clientSocket, char *handle, int len);
void sendClientHandleFinPacket(int clientSocket);
void handleClientHandleRequest(int clientSocket, Nodelist *list);
void handleSocket(int clientSocket, Nodelist *list);
void handleIncomingRequests(int serverSocket);
