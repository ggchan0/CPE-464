#include "connections.h"
#include "gethostbyname6.h"

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

void sendPacket(int socketNum, char * sendBuf, int len) {
	int sent = 0;

	if ((sent = send(socketNum, sendBuf, len, 0)) < 0) {
		perror("client init send");
		exit(-1);
	}
}

void createInitPacket(char * buf, char * clientHandle, uint16_t * len) {
	uint8_t handleLen = strlen(clientHandle);
	uint8_t flag = INIT_PACKET;
	int offset = 0;
	uint16_t packetLen = htons(PDU_LEN_SIZE + FLAG_SIZE + HANDLE_LEN_SIZE + handleLen);
	*len = ntohs(packetLen);

	printf("packetLen %u %u\n", *len, handleLen);

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

void handleIncomingRequests(int serverSocket) {
	Nodelist *list = initializeNodelist();
	fd_set socketList;

	while(1) {
		ClientNode *temp = list->head;
		int highestFD = serverSocket;
		int active;

		printf("round 1\n");
		FD_ZERO(&socketList);
		FD_SET(serverSocket, &socketList);
		while (temp != NULL) {
			printf("going through linkedlist\n");
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
			printf("handling socket %d\n", clientSocket);
			//handleClientInit(clientSocket, list);
		}
		/*
		temp = list->head;

		while (temp != NULL) {
			if (FD_ISSET(temp->socketNum, &socketList))
			printf("%d\n", temp->socketNum);
			exit(0);
		}
		*/



	}
}

int acceptClient(int serverSocket) {
	struct sockaddr_in6 clientInfo;
	int clientInfoSize = sizeof(clientInfo);
	int clientSocket;

	if ((clientSocket = accept(serverSocket, (struct sockaddr *) &clientInfo, (socklen_t *) &clientInfoSize)) < 0) {
		perror("server accept");
		exit(-1);
	}

	printf("accepted\n");

	return clientSocket;
}

int duplicateHandle(char * handle, Nodelist * list) {
	ClientNode *temp = list->head;
	while (temp != NULL) {
		if (strcmp(temp->handle, handle) == 0) {
			return 1;
		} else {
			temp = temp->next;
		}
	}

	return 0;
}

void sendClientInitErrorPacket(int clientSocket) {

}

void handleClientInit(int clientSocket, Nodelist *list) {
	char buf[MAXBUF];
	int messageLen = 0;
	uint8_t handleLen = 0;
	char handle[100];

	if ((messageLen = recv(clientSocket, buf, MAXBUF, MSG_WAITALL)) < 0) {
		perror("server recv init");
		exit(-1);
	}

	memcpy(&handleLen, buf + HANDLE_POS, HANDLE_LEN_SIZE);
	memcpy(handle, buf + HANDLE_POS + 1, handleLen);
	handle[handleLen] = 0;

	if (duplicateHandle(handle, list)) {
		sendClientInitErrorPacket(clientSocket);
		close(clientSocket);
	} else {
		ClientNode *node = initializeClientNode(handle, clientSocket);
		printf("%s\n", handle);
		addToNodelist(list, node);
	}

}
