#include "connections.h"
#include "gethostbyname6.h"

void sendPacket(int socketNum, char * sendBuf, int len) {
	int sent = 0;

	if ((sent = send(socketNum, sendBuf, len, 0)) < 0) {
		perror("send error");
		exit(-1);
	}
}

void readFromSocket(int socketNum, char * buf, int * messageLen) {
	int bytesRead = 0;
	int bytesLeft = 0;
	uint16_t packetLen = 0;

	
	if ((bytesRead = recv(socketNum, buf, PDU_LEN_SIZE, 0)) < 0) {
		perror("recv error");
		exit(-1);
	}

	if (bytesRead == 0) {
		*messageLen = packetLen;
		return;
	}

	memcpy(&packetLen, buf, PDU_LEN_SIZE);
	packetLen = ntohs(packetLen);

	bytesLeft = packetLen - bytesRead;
	
	while (bytesLeft > 0) {
		if ((bytesRead = recv(socketNum, buf + (packetLen - bytesLeft), bytesLeft, 0)) < 0) {
			perror("recv error");
			exit(-1);
		}

		bytesLeft -= bytesRead;
	}

	*messageLen = packetLen;
}

char * doubleSize(char *str, int length) {
   return realloc(str, length);
}

char *readline(FILE *file) {
   int length = 10;
   int index = 0;
   char *str = malloc(sizeof(char) * length);
   int c;
   while ((c = fgetc(file)) != EOF && c != '\n') {
      str[index++] = c;
      if (index == (length - 1)) {
         length *= 2;
         str = doubleSize(str, length);
      }
   }
   str[index] = '\0';
   if (c == EOF) {
      free(str);
      str = NULL;
   }
   return str;
}

int setupClient(char * serverName, int port) {
	int socketNum;
	uint8_t *ipAddress = NULL;
	struct sockaddr_in6 server;

	if ((socketNum = socket(AF_INET6, SOCK_STREAM, 0)) < 0) {
		perror("client socket");
		exit(-1);
	}

	server.sin6_family = AF_INET6;
	server.sin6_port = htons(port);

	if ((ipAddress = getIPAddress6(serverName, &server)) == NULL) {
		exit(-1);
	}

	if (connect(socketNum, (struct sockaddr*) &server, sizeof(server)) < 0) {
		perror("client connect");
		exit(-1);
	}

	return socketNum;
}

void createInitPacket(char * buf, char * clientHandle, uint16_t * len) {
	uint8_t handleLen = strlen(clientHandle);
	uint8_t flag = INIT_PACKET;
	int offset = 0;
	uint16_t packetLen = htons(PDU_LEN_SIZE + FLAG_SIZE + HANDLE_LEN_SIZE + handleLen);
	*len = ntohs(packetLen);

	memcpy(buf + offset, &packetLen, PDU_LEN_SIZE);
	offset += PDU_LEN_SIZE;
	memcpy(buf + offset, &flag, FLAG_SIZE);
	offset += FLAG_SIZE;
	memcpy(buf + offset, &handleLen, HANDLE_LEN_SIZE);
	offset += HANDLE_LEN_SIZE;
	memcpy(buf + offset, clientHandle, handleLen);
}

void sendInitPacketToServer(int socketNum, char * clientHandle) {
	char sendBuf[MAXBUF];
	uint16_t len = 0;
	createInitPacket(sendBuf, clientHandle, &len);
	sendPacket(socketNum, sendBuf, len);
}

void recvConfirmationFromServer(int serverSocket, char * clientHandle) {
	char buf[MAXBUF];
	int messageLen = 0;
	uint8_t flag = 0;

	readFromSocket(serverSocket, buf, &messageLen);	

	if (messageLen == 0) {
		printf("Server exited\n");
		exit(0);
	}

	memcpy(&flag, buf + FLAG_POS, FLAG_SIZE);

	if (flag == BAD_HANDLE) {
		fprintf(stderr, "Handle already in use, %s\n", clientHandle);
		exit(-1);
	} else if (flag != GOOD_HANDLE) {
		fprintf(stderr, "Incorrect packet return type, terminating\n");
		exit(-1);
	} 
}

void sendMessagePacket(int serverSocket, char * handle, char * destBuf, int destBufSize, char * buf, int bytesCopied) {
	char sendBuf[MAXBUF];
	uint8_t handleLen = strlen(handle);
	uint16_t packetLen = htons(CHAT_HEADER_SIZE + HANDLE_LEN_SIZE + handleLen + destBufSize + bytesCopied);
	uint8_t flag = C_TO_C;
	int offset = 0;

	memcpy(sendBuf + PDU_POS, &packetLen, PDU_LEN_SIZE);
	memcpy(sendBuf + FLAG_POS, &flag, FLAG_SIZE);
	offset += CHAT_HEADER_SIZE;
	memcpy(sendBuf + offset, &handleLen, HANDLE_LEN_SIZE);
	offset += HANDLE_LEN_SIZE;
	memcpy(sendBuf + offset, handle, handleLen);
	offset += handleLen;
	memcpy(sendBuf + offset, destBuf, destBufSize);
	offset += destBufSize;
	memcpy(sendBuf + offset, buf, bytesCopied);

	sendPacket(serverSocket, sendBuf, ntohs(packetLen));
}

void messageServer(int serverSocket, char * handle, char * destBuf, int destBufSize, char * text) {
	int counter = 0;
	int bytesLeft = strlen(text);
	while (bytesLeft > 0) {
		char buf[MAX_TEXT_SIZE + 1];
		int bytesCopied = 0;
		if (bytesLeft >= MAX_TEXT_SIZE) {
			memcpy(buf, text + counter, MAX_TEXT_SIZE);
			counter += MAX_TEXT_SIZE;
			bytesLeft -= MAX_TEXT_SIZE;
			bytesCopied = MAX_TEXT_SIZE;
		} else {
			memcpy(buf, text + counter, bytesLeft);
			counter += bytesLeft;
			bytesCopied = bytesLeft;
			bytesLeft = 0;
		}
		buf[bytesCopied] = 0;
		sendMessagePacket(serverSocket, handle, destBuf, destBufSize, buf, bytesCopied + 1);
	}
}

void parseAndSendMessage(int serverSocket, char * handle, char * buf) {
	char *space = " ";
	char *empty = "";
	char *token = NULL;
	char *text = NULL;
	uint8_t numDest = 0;

	token = strtok(buf, space);

	numDest = atoi(token);

	if (numDest == 0) {
		char destBuf[MAXBUF];
		char *target = token;
		uint8_t destHandleLen = strlen(target);
		int destBufSize = 0;
		numDest = 1;
		memcpy(destBuf, &numDest, HANDLE_LEN_SIZE);
		destBufSize += HANDLE_LEN_SIZE;
		memcpy(destBuf + HANDLE_LEN_SIZE, &destHandleLen, HANDLE_LEN_SIZE);
		destBufSize += HANDLE_LEN_SIZE;
		memcpy(destBuf + HANDLE_LEN_SIZE * 2, target, (int) destHandleLen);
		destBufSize += destHandleLen;
		
		text = strtok(NULL, empty);
		if (text == NULL) {
			text = "\n";
		}

		messageServer(serverSocket, handle, destBuf, destBufSize, text);
	} else if (numDest > 9 || numDest < 0) {
		fprintf(stderr, "Invalid number of handles\n");
	} else {
		int i;
		int offset = 0;
		char destBuf[MAXBUF];
		memcpy(destBuf, &numDest, HANDLE_LEN_SIZE);
		offset += HANDLE_LEN_SIZE;
		for (i = 0; i < numDest; i++) {
			char * dest = strtok(NULL, space);
			uint8_t destLen = strlen(dest);
			memcpy(destBuf + offset, &destLen, HANDLE_LEN_SIZE);
			offset += HANDLE_LEN_SIZE;
			memcpy(destBuf + offset, dest, (int) destLen);
			offset += destLen;
		}
		text = strtok(NULL, empty);
		if (text == NULL) {
			text = "\n";
		}

		messageServer(serverSocket, handle, destBuf, offset, text);
	}
}

void sendBroadcastPacket(int serverSocket, char * handle, char * buf, int bytesCopied) {
	char sendBuf[MAXBUF];
	uint8_t handleLen = strlen(handle);
	uint16_t packetLen = htons(CHAT_HEADER_SIZE + HANDLE_LEN_SIZE + handleLen + bytesCopied);
	uint8_t flag = BROADCAST_MESSAGE;
	int offset = 0;

	memcpy(sendBuf + PDU_POS, &packetLen, PDU_LEN_SIZE);
	memcpy(sendBuf + FLAG_POS, &flag, FLAG_SIZE);
	offset += CHAT_HEADER_SIZE;
	memcpy(sendBuf + offset, &handleLen, HANDLE_LEN_SIZE);
	offset += HANDLE_LEN_SIZE;
	memcpy(sendBuf + offset, handle, handleLen);
	offset += handleLen;
	memcpy(sendBuf + offset, buf, bytesCopied);

	sendPacket(serverSocket, sendBuf, ntohs(packetLen));
}

void broadcastToServer(int serverSocket, char * handle, char * text) {
	int counter = 0;
	int bytesLeft = strlen(text);

	while (bytesLeft > 0) {
		char buf[MAX_TEXT_SIZE + 1];
		int bytesCopied = 0;
		if (bytesLeft >= MAX_TEXT_SIZE) {
			memcpy(buf, text + counter, MAX_TEXT_SIZE);
			counter += MAX_TEXT_SIZE;
			bytesLeft -= MAX_TEXT_SIZE;
			bytesCopied = MAX_TEXT_SIZE;
		} else {
			memcpy(buf, text + counter, bytesLeft);
			counter += bytesLeft;
			bytesCopied = bytesLeft;
			bytesLeft = 0;
		}
		buf[bytesCopied] = 0;
		sendBroadcastPacket(serverSocket, handle, buf, bytesCopied + 1);
	}
}

void createListHandlesPacket(char * buf) {
	uint16_t packetLen = htons(CHAT_HEADER_SIZE);
	uint8_t flag = HANDLE_REQ;

	memcpy(buf, &packetLen, PDU_LEN_SIZE);
	memcpy(buf + FLAG_POS, &flag, FLAG_SIZE);
}

void sendListHandlesPacketToServer(int socketNum) {
	char sendBuf[MAXBUF];
	createListHandlesPacket(sendBuf);
	sendPacket(socketNum, sendBuf, CHAT_HEADER_SIZE);
}

void createExitPacket(char * buf) {
	uint16_t packetLen = htons(CHAT_HEADER_SIZE);
	uint8_t flag = C_EXIT;

	memcpy(buf, &packetLen, PDU_LEN_SIZE);
	memcpy(buf + FLAG_POS, &flag, FLAG_SIZE);
}

void sendExitPacketToServer(int socketNum) {
	char sendBuf[MAXBUF];
	createExitPacket(sendBuf);
	sendPacket(socketNum, sendBuf, CHAT_HEADER_SIZE);
}

void processInput(char * str, char * handle, int serverSocket) {
	char *space = " ";
	char *empty = "";
	char *token = NULL;
	char *text = NULL;
	int tokenNum = 1;
	char cmd = 0;

	token = strtok(str, space);

	if (token != NULL) {
		if (tokenNum++ == 1) {
			if (token[0] != '%') {
				printf("Invalid command\n");
				return;
			}
			cmd = tolower(token[1]);
		}

		switch(cmd) {
			case 'm':
				parseAndSendMessage(serverSocket, handle, strtok(NULL, empty));
				break;
			case 'b':
				text = strtok(NULL, empty);
				if (text == NULL) {
					text = "\n";
				}
				broadcastToServer(serverSocket, handle, text);
				break;
			case 'l':
				sendListHandlesPacketToServer(serverSocket);
				break;
			case 'e':
				sendExitPacketToServer(serverSocket);
				break;
			default:
				printf("Invalid command\n");
				return;
				break;
		}

		token = strtok(NULL, space);
	}
}

void receiveBroadcastMessage(char * buf) {
	char handle[MAX_HANDLE_SIZE];
	uint8_t handleLen = 0;
	int offset = HANDLE_POS;
	
	memcpy(&handleLen, buf + offset, HANDLE_LEN_SIZE);
	offset += HANDLE_LEN_SIZE;
	memcpy(&handle, buf + offset, handleLen);
	offset += handleLen;
	printf("%s: %s\n", handle, buf + offset);
}

void receiveMessage(char * buf) {
	uint8_t handleLen = 0;
	uint8_t numHandles = 0;
	char handle[MAX_HANDLE_SIZE];
	int offset = 0;
	int i = 0;

	offset += HANDLE_POS;
	memcpy(&handleLen, buf + HANDLE_POS, HANDLE_LEN_SIZE);
	offset += HANDLE_LEN_SIZE;
	memcpy(handle, buf + HANDLE_LABEL_START, (int) handleLen);
	handle[handleLen] = 0;
	offset += handleLen;
	memcpy(&numHandles, buf + offset, HANDLE_LEN_SIZE);
	offset += HANDLE_LEN_SIZE;

	for (i = 0; i < numHandles; i++) {
		uint8_t destLen = 0;

		memcpy(&destLen, buf + offset, HANDLE_LEN_SIZE);
		offset += destLen + HANDLE_LEN_SIZE;
	}

	printf("%s: %s\n", handle, buf + offset);
}

void receiveErrorPacket(char * buf) {
	uint8_t handleLen = 0;
	char handle[MAX_HANDLE_SIZE];

	memcpy(&handleLen, buf + HANDLE_POS, HANDLE_LEN_SIZE);

	memcpy(handle, buf + HANDLE_LABEL_START, (int) handleLen);
	handle[handleLen] = 0;

	printf("Client with handle %s does not exist\n", handle);
}

void processHandle(int serverSocket) {
	uint8_t handleLen = 0;
	int messageLen = 0;
	char buf[MAXBUF];
	char handle[MAX_HANDLE_SIZE];

	readFromSocket(serverSocket, buf, &messageLen);

	if (messageLen == 0) {
		printf("Server terminated\n");
		exit(0);
	}

	memcpy(&handleLen, buf + HANDLE_POS, HANDLE_LEN_SIZE);

	memcpy(handle, buf + HANDLE_LABEL_START, handleLen);
	handle[handleLen] = 0;

	printf("\t%s\n", handle);
}

void processHandleReqFin(int serverSocket) {
	char buf[MAXBUF];
	int messageLen = 0;
	uint8_t flag = 0;

	readFromSocket(serverSocket, buf, &messageLen);

	if (messageLen == 0) {
		printf("Server terminated\n");
		exit(0);
	}

	memcpy(&flag, buf + FLAG_POS, FLAG_SIZE);

	if (flag != HANDLE_REQ_FIN) {
		fprintf(stderr, "Bad handle request finish from server\n");
	}
}

void processListHandles(int serverSocket, char * buf) {
	uint32_t numHandles = 0;

	memcpy(&numHandles, buf + HANDLE_POS, HANDLE_RES_SIZE);

	printf("Number of clients: %u\n", ntohl(numHandles));
	for (int i = 0; i < ntohl(numHandles); i++) {
		processHandle(serverSocket);
	}

	processHandleReqFin(serverSocket);
}

void processMessage(int serverSocket) {
	char buf[MAXBUF];
	int messageLen = 0;
	uint8_t flag = 0;

	readFromSocket(serverSocket, buf, &messageLen);

	if (messageLen == 0) {
		printf("Server terminated\n");
		exit(0);
	}

	memcpy(&flag, buf + FLAG_POS, FLAG_SIZE);

	switch(flag) {
		case BROADCAST_MESSAGE:
			receiveBroadcastMessage(buf);
			break;
		case C_TO_C:
			receiveMessage(buf);
			break;
		case ERR_PACKET:
			receiveErrorPacket(buf);
			break;
		case C_EXIT_ACK:
			printf("Exiting\n");
			exit(0);
			break;
		case NUM_HANDLE_RES:
			processListHandles(serverSocket, buf);
			break;
		default:
			break;
	}
}

void enterInteractiveMode(char * clientHandle, int serverSocket) {
	fd_set fdList;

	while(1) {
		int highestFD = serverSocket;
		int active;

		FD_ZERO(&fdList);
		FD_SET(STDIN_FILENO, &fdList);
		FD_SET(serverSocket, &fdList);
		printf("$: ");
		fflush(stdout);
		active = select(highestFD + 1, &fdList, NULL, NULL, NULL);

		if (active < 0) {
			perror("client select");
			exit(-1);
		}

		

		if (FD_ISSET(STDIN_FILENO, &fdList)) {
			char *str = readline(stdin);
			if (str == NULL) {
			} else {
				processInput(str, clientHandle, serverSocket);
			}
			free(str);
		}

		if (FD_ISSET(serverSocket, &fdList)) {
			processMessage(serverSocket);
		}
	}
}

int setupServer(int port) {
	int serverSocket = 0;
	struct sockaddr_in6 server;
	socklen_t len = sizeof(server);

	if ((serverSocket = socket(AF_INET6, SOCK_STREAM, 0)) < 0) {
		perror("server socket");
		exit(1);
	}

	server.sin6_family = AF_INET6;
	server.sin6_addr = in6addr_any;
	server.sin6_port = htons(port);

	if (bind(serverSocket, (struct sockaddr *) &server, sizeof(server)) < 0) {
		perror("server bind");
		exit(-1);
	}

	if (getsockname(serverSocket, (struct sockaddr *) &server, &len) < 0) {
		perror("server getsockname");
		exit(-1);
	}

   if (listen(serverSocket, BACKLOG) < 0) {
		perror("server listen");
		exit(-1);
	}

	printf("Server is using port %d \n", ntohs(server.sin6_port));

	return serverSocket;
}

int acceptClient(int serverSocket) {
	struct sockaddr_in6 clientInfo;
	int clientInfoSize = sizeof(clientInfo);
	int clientSocket;

	if ((clientSocket = accept(serverSocket, (struct sockaddr *) &clientInfo, (socklen_t *) &clientInfoSize)) < 0) {
		perror("server accept");
		exit(-1);
	}

	return clientSocket;
}

void handleClientExit(int clientSocket, Nodelist * list) {
	removeNode(list, clientSocket);
	close(clientSocket);
}

int duplicateHandle(char * handle, Nodelist * list) {
	ClientNode *temp = list->head;
	while (temp != NULL) {
		if (temp->handle != NULL && strcmp(temp->handle, handle) == 0) {
			return 1;
		} else {
			temp = temp->next;
		}
	}

	return 0;
}

void sendClientInitErrorPacket(int clientSocket) {
	uint16_t packetSize = htons(CHAT_HEADER_SIZE);
	uint8_t flag = BAD_HANDLE;
	char buf[CHAT_HEADER_SIZE];

	memcpy(buf, &packetSize, PDU_LEN_SIZE);
	memcpy(buf + FLAG_POS, &flag, FLAG_SIZE);

	sendPacket(clientSocket, buf, ntohs(packetSize));
}

void sendClientInitSuccessPacket(int clientSocket) {
	uint16_t packetSize = htons(CHAT_HEADER_SIZE);
	uint8_t flag = GOOD_HANDLE;
	char buf[CHAT_HEADER_SIZE];

	memcpy(buf, &packetSize, PDU_LEN_SIZE);
	memcpy(buf + FLAG_POS, &flag, FLAG_SIZE);

	sendPacket(clientSocket, buf, ntohs(packetSize));
}

void handleClientInit(int clientSocket, char * buf, Nodelist *list) {
	uint8_t handleLen = 0;
	char handle[100];

	memcpy(&handleLen, buf + HANDLE_POS, HANDLE_LEN_SIZE);
	memcpy(handle, buf + HANDLE_POS + 1, handleLen);
	handle[handleLen] = 0;

	if (duplicateHandle(handle, list)) {
		sendClientInitErrorPacket(clientSocket);
		removeNode(list, clientSocket);
		close(clientSocket);
	} else {
		ClientNode *node = findNode(list, clientSocket);
		node->handle = strdup(handle);
		sendClientInitSuccessPacket(clientSocket);
	}

}

void handleInvalidHandle(int clientSocket, char * dest) {
	char buf[MAXBUF];
	uint8_t flag = ERR_PACKET;
	uint8_t handleLen = strlen(dest);
	uint16_t packetLen = htons(CHAT_HEADER_SIZE + HANDLE_LEN_SIZE + handleLen);

	memcpy(buf, &packetLen, PDU_LEN_SIZE);
	memcpy(buf + FLAG_POS, &flag, FLAG_SIZE);
	memcpy(buf + HANDLE_POS, &handleLen, HANDLE_LEN_SIZE);
	memcpy(buf + HANDLE_LABEL_START, dest, handleLen);

	sendPacket(clientSocket, buf, (int) ntohs(packetLen));

}

void handleMessage(int clientSocket, char * buf, Nodelist *list) {
	char handle[MAX_HANDLE_SIZE];
	uint16_t packetLen = 0;
	uint8_t handleLen = 0;
	uint8_t numDests = 0;
	int offset = 0;

	memcpy(&packetLen, buf, PDU_LEN_SIZE);

	packetLen = ntohs(packetLen);

	offset += CHAT_HEADER_SIZE;
	memcpy(&handleLen, buf + offset, HANDLE_LEN_SIZE);
	offset += HANDLE_LEN_SIZE;

	memcpy(handle, buf + offset, handleLen);
	handle[handleLen] = 0;

	offset += handleLen;
	memcpy(&numDests, buf + offset, HANDLE_LEN_SIZE);
	offset += HANDLE_LEN_SIZE;

	for (int i = 0; i < numDests; i++) {
		char dest[MAX_HANDLE_SIZE];
		uint8_t tempHandleLen = 0;
		ClientNode *temp = list->head;
		int invalidHandle = 1;
		memcpy(&tempHandleLen, buf + offset, HANDLE_LEN_SIZE);
		offset += HANDLE_LEN_SIZE;
		memcpy(dest, buf + offset, tempHandleLen);
		dest[tempHandleLen] = 0;
		offset += tempHandleLen;
		while (temp != NULL) {
			if (strcmp(temp->handle, dest) == 0) {
				sendPacket(temp->socketNum, buf, (int) packetLen);
				invalidHandle = 0;
			}
			temp = temp->next;
		}

		if (invalidHandle) {
			handleInvalidHandle(clientSocket, dest);
		}
	}

}

void handleBroadcast(int clientSocket, char * buf, Nodelist *list) {
	char handle[MAX_HANDLE_SIZE];
	uint16_t packetLen = 0;
	uint8_t handleLen = 0;
	int offset = 0;
	ClientNode *temp = list->head;

	memcpy(&packetLen, buf, PDU_LEN_SIZE);

	packetLen = ntohs(packetLen);

	offset += CHAT_HEADER_SIZE;
	memcpy(&handleLen, buf + offset, HANDLE_LEN_SIZE);
	offset += HANDLE_LEN_SIZE;

	memcpy(handle, buf + offset, handleLen);
	handle[handleLen] = 0;

	while (temp != NULL) {
		if (strcmp(temp->handle, handle) != 0) {
			sendPacket(temp->socketNum, buf, (int) packetLen);
		}
		temp = temp->next;
	}	
}

void sendClientExitAckPacket(int clientSocket) {
	uint16_t packetSize = htons(CHAT_HEADER_SIZE);
	uint8_t flag = C_EXIT_ACK;
	char buf[CHAT_HEADER_SIZE];

	memcpy(buf, &packetSize, PDU_LEN_SIZE);
	memcpy(buf + FLAG_POS, &flag, FLAG_SIZE);

	sendPacket(clientSocket, buf, ntohs(packetSize));
}

void sendClientHandleResPacket(int clientSocket, Nodelist *list) {
	char buf[CHAT_HEADER_SIZE + HANDLE_RES_SIZE];
	uint16_t packetSize = htons(CHAT_HEADER_SIZE + HANDLE_RES_SIZE);
	uint8_t flag = NUM_HANDLE_RES;
	uint32_t numHandles = 0;
	ClientNode *temp = list->head;
	int offset = 0;

	while (temp != NULL) {
		numHandles++;
		temp = temp->next;
	}

	numHandles = htonl(numHandles);

	memcpy(buf, &packetSize, PDU_LEN_SIZE);
	memcpy(buf + FLAG_POS, &flag, FLAG_SIZE);
	offset += CHAT_HEADER_SIZE;
	memcpy(buf + offset, &numHandles, HANDLE_RES_SIZE);

	sendPacket(clientSocket, buf, ntohs(packetSize));
}

void sendClientHandlePacket(int clientSocket, char *handle, int len) {
	uint8_t handleLen = len;
	uint8_t flag = HANDLE_RES;
	uint16_t packetSize = htons(CHAT_HEADER_SIZE + HANDLE_LEN_SIZE + handleLen);
	char buf[MAXBUF];
	int offset = 0;

	memcpy(buf, &packetSize, PDU_LEN_SIZE);
	memcpy(buf + FLAG_POS, &flag, FLAG_SIZE);
	offset += CHAT_HEADER_SIZE;
	memcpy(buf + offset, &handleLen, HANDLE_LEN_SIZE);
	offset += HANDLE_LEN_SIZE;
	memcpy(buf + offset, handle, len);

	sendPacket(clientSocket, buf, ntohs(packetSize));
}

void sendClientHandleFinPacket(int clientSocket) {
	uint8_t flag = HANDLE_REQ_FIN;
	uint16_t packetSize = htons(CHAT_HEADER_SIZE);
	char buf[CHAT_HEADER_SIZE];

	memcpy(buf, &packetSize, PDU_LEN_SIZE);
	memcpy(buf + FLAG_POS, &flag, FLAG_SIZE);

	sendPacket(clientSocket, buf, ntohs(packetSize));
}

void handleClientHandleRequest(int clientSocket, Nodelist *list) {
	ClientNode *temp = list->head;
	
	sendClientHandleResPacket(clientSocket, list);
	while (temp != NULL) {
		sendClientHandlePacket(clientSocket, temp->handle, strlen(temp->handle));
		temp = temp->next;
	}

	sendClientHandleFinPacket(clientSocket);
}

void handleSocket(int clientSocket, Nodelist *list) {
	char buf[MAXBUF];
	int messageLen = 0;
	uint8_t flag = 0;

	readFromSocket(clientSocket, buf, &messageLen);

	if (messageLen == 0) {
		handleClientExit(clientSocket, list);
		return;
	}

	memcpy(&flag, buf + FLAG_POS, FLAG_SIZE);

	switch(flag) {
		case INIT_PACKET:
			handleClientInit(clientSocket, buf, list);
			break;
		case BROADCAST_MESSAGE:
			handleBroadcast(clientSocket, buf, list);
			break;
		case C_TO_C:
			handleMessage(clientSocket, buf, list);
			break;
		case C_EXIT:
			sendClientExitAckPacket(clientSocket);
			handleClientExit(clientSocket, list);
			break;
		case HANDLE_REQ:
			handleClientHandleRequest(clientSocket, list);
			break;
		default:
			break;
	}

}

void handleIncomingRequests(int serverSocket) {
	Nodelist *list = initializeNodelist();
	fd_set socketList;

	while(1) {
		ClientNode *temp = list->head;
		int highestFD = serverSocket;
		int active;

		FD_ZERO(&socketList);
		FD_SET(serverSocket, &socketList);
		while (temp != NULL) {
			FD_SET(temp->socketNum, &socketList);
			if (temp->socketNum > highestFD) {
				highestFD = temp->socketNum;
			}
			temp = temp->next;
		}

		active = select(highestFD + 1, &socketList, NULL, NULL, NULL);
		if (active < 0) {
			perror("server select");
			exit(-1);
		}

		if (FD_ISSET(serverSocket, &socketList)) {
			int clientSocket = acceptClient(serverSocket);
			ClientNode *newClient = initializeClientNode(clientSocket);
			addToNodelist(list, newClient);
		}

		temp = list->head;

		while (temp != NULL) {
			if (FD_ISSET(temp->socketNum, &socketList)) {
				handleSocket(temp->socketNum, list);
			}
			temp = temp->next;
		}
	}
}
