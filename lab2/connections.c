#include "connections.h"
#include "gethostbyname6.h"

void sendPacket(int socketNum, char * sendBuf, int len) {
	int sent = 0;

	if ((sent = send(socketNum, sendBuf, len, 0)) < 0) {
		perror("client init send");
		exit(-1);
	}
}

void readFromSocket(int socketNum, char * buf, int * messageLen) {
	int bytesRead = 0;

	if ((bytesRead = recv(socketNum, buf, MAXBUF, 0)) < 0) {
		perror("server recv handling client");
		exit(-1);
	}

	*messageLen = bytesRead;
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

void recvConfirmationFromServer(int serverSocket) {
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
		fprintf(stderr, "Handle already in use, terminating\n");
		exit(-1);
	} else if (flag != GOOD_HANDLE) {
		fprintf(stderr, "Incorrect packet return type, terminating\n");
		exit(-1);
	} 
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
	printf("sent exit packet\n");
}

void processInput(char * str, char * handle, int serverSocket) {
	char *space = " ";
	char *token;
	int tokenNum = 1;
	char cmd;

	token = strtok(str, space);

	while (token != NULL) {
		printf("tok %s\n", token);
		if (tokenNum++ == 1) {
			cmd = tolower(token[1]);
			printf("%c\n", cmd);
		}

		switch(cmd) {
			case 'm':

				break;
			case 'b':

				break;
			case 'l':

				break;
			case 'e':
				sendExitPacketToServer(serverSocket);
				break;
			default:

				break;
		}
		break;

		token = strtok(NULL, space);
	}
}

void processMessage(int serverSocket) {
	char buf[MAXBUF];
	int messageLen = 0;
	uint8_t flag = 0;

	printf("reading something from server\n");

	readFromSocket(serverSocket, buf, &messageLen);

	printf("read something from server\n");

	if (messageLen == 0) {
		printf("Server exited\n");
		exit(0);
	}

	memcpy(&flag, buf + FLAG_POS, FLAG_SIZE);

	switch(flag) {
		case BROADCAST_MESSAGE:
			break;
		case C_TO_C:
			break;
		case ERR_PACKET:
			break;
		case C_EXIT_ACK:
			printf("Exiting\n");
			exit(0);
			break;
		case NUM_HANDLE_RES:
			break;
		case HANDLE_RES:
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

	printf("Server Port Number %d \n", ntohs(server.sin6_port));

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

	printf("checking handles\n");

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

void sendClientExitAckPacket(int clientSocket) {
	uint16_t packetSize = htons(CHAT_HEADER_SIZE);
	uint8_t flag = C_EXIT_ACK;
	char buf[CHAT_HEADER_SIZE];

	memcpy(buf, &packetSize, PDU_LEN_SIZE);
	memcpy(buf + FLAG_POS, &flag, FLAG_SIZE);

	sendPacket(clientSocket, buf, ntohs(packetSize));
}

void sendClientHandleResPacket(int clientSocket, Nodelist *list) {
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

	char buf[CHAT_HEADER_SIZE + HANDLE_RES_SIZE];

	memcpy(buf, &packetSize, PDU_LEN_SIZE);
	memcpy(buf + FLAG_POS, &flag, FLAG_SIZE);
	offset += CHAT_HEADER_SIZE;
	memcpy(buf + offset, &numHandles, HANDLE_RES_SIZE);

	sendPacket(clientSocket, buf, ntohs(packetSize));
}

void sendClientHandlePacket(int clientSocket, char *handle, int len) {
	uint8_t = strlen(node->handle);
	uint16_t packetSize;
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

	printf("Handling socket %d\n", clientSocket);

	readFromSocket(clientSocket, buf, &messageLen);

	printf("Read from socket\n");

	if (messageLen == 0) {
		printf("client exit\n");
		handleClientExit(clientSocket, list);
		return;
	}

	memcpy(&flag, buf + FLAG_POS, FLAG_SIZE);

	switch(flag) {
		case INIT_PACKET:
			handleClientInit(clientSocket, buf, list);
			break;
		case BROADCAST_MESSAGE:
			break;
		case C_TO_C:
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
		printf("loop\n");
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
			printf("adding new socket\n");
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
