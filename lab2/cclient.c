#include "cclient.h"
#include "connections.h"

void checkArgs(int argc, char *argv[]) {

	if (argc != 4)
	{
		fprintf(stderr, "Usage: cclient <handle> <server-name> <server-port>\n");
		exit(-1);
	}

	if (strlen(argv[1]) > 100) {
		fprintf(stderr, "Invalid handle, handle longer than 100 characters: %s\n", argv[1]);
		exit(-1);
	}
}

void checkHandle(char * clientHandle) {
	if (isdigit(clientHandle[0])) {
		fprintf(stderr, "Invalid handle, handle starts with a number\n");
		exit(-1);
	}
}

int main(int argc, char * argv[]) {
	char *clientHandle;
	int serverSocket;

	checkArgs(argc, argv);
	clientHandle = argv[1];
	serverSocket = setupClient(argv[2], atoi(argv[3]));

	checkHandle(clientHandle);

	sendInitPacketToServer(serverSocket, clientHandle);
	
	recvConfirmationFromServer(serverSocket, clientHandle);
	
	enterInteractiveMode(clientHandle, serverSocket);
	
	return 0;
}
