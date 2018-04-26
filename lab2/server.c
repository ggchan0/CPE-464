#include "connections.h"
#include "server.h"

void checkArgs(int argc, char *argv[]) {
	if (argc > 2) {
		fprintf(stderr, "Usage %s [optional port number]\n", argv[0]);
		exit(-1);
	}
}

int getPortFromArgs(int argc, char *argv[]) {
	int portNumber = 0;
	if (argc == 2) {
		portNumber = atoi(argv[1]);
	}

	return portNumber;
}

int main(int argc, char * argv[]) {
   int serverSocket = 0;
	int portNumber;

	checkArgs(argc, argv);
	portNumber = getPortFromArgs(argc, argv);

	serverSocket = setupServer(portNumber);

	handleIncomingRequests(serverSocket);
	return 0;
}
