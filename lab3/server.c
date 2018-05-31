/*
    Code borrowed from Professor Hugh Smith
    CPE-464 Spring 2018
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/uio.h>
#include <sys/time.h>
#include <fcntl.h>
#include <ctype.h>
#include <string.h>
#include <strings.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#include "cpe464.h"
#include "connections.h"

typedef enum State STATE;

enum State {
    START, DONE, FILENAME, SEND_DATA, WAIT_ON_ACK, TIMEOUT_ON_ACK, WAIT_ON_EOF_ACK, TIMEOUT_ON_EOF_ACK
}

int processArgs(int argc, char **argv);

int main(int argc, char **argv) {
    int sk_num = 0;
    int portNumber = 0;

    portNumber = processArgs(argc, argv);
    sendToErr_init(atof(argv[1]), DROP_ON, FLIP_ON, DEBUG_ON, RSEED_ON);

    sk_num = udpServerSetup(portNumber);

    processServer(sk_num);

    return 0;
}

int processArgs(int argc, char **argv) {
    int portNumber = 0;

    if (argc < 2 || argc > 3) {
        printf("Usage %s error_rate [port_number]\n", argv[0]);
        exit(-1);
    }

    if (argc == 3) {
        portNumber = atoi(argv[2]);
    } else {
        portNumber = 0;
    }

    return portNumber;
}
