#include "cclient.h"
#include "connections.h"

void checkArgs(int argc, char *argv[]) {

	if (argc != 4)
	{
		fprintf(stderr, "Usage: cclient <handle> <server-name> <server-port>\n");
		exit(-1);
	}

	if (strlen(argv[1]) > 100) {
		fprintf(stderr, "Maximum handle length is 100 characters\n");
		exit(-1);
	}
}

int main(int argc, char * argv[]) {
	char *clientHandle;
	int serverSocket;

	checkArgs(argc, argv);
	clientHandle = argv[1];
	serverSocket = setupClient(argv[2], atoi(argv[3]));

	sendInitPacketToServer(serverSocket, clientHandle);
	
	recvConfirmationFromServer(serverSocket);
	
	enterInteractiveMode(clientHandle, serverSocket);
	
	return 0;
}
