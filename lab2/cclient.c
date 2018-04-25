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
	char *serverName;
	int serverPort;
	int socketNum;

	checkArgs(argc, argv);
	clientHandle = argv[1];
	serverName = argv[2];
	serverPort = atoi(argv[3]);
	socketNum = setupClient(serverName, serverPort);

	printf("%s %d\n", clientHandle, socketNum);


	sendInitPacketToServer(socketNum, clientHandle);

	/*

	recvConfirmationFromServer(serverName, serverPort);

	enterInteractiveMode(clientHandle, serverName, serverPort);
	*/
	return 0;
}
