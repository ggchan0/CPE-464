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
#include "networks.h"

typedef enum State STATE;

enum State {
    DONE, FILENAME, RECV_DATA, FILE_OK, START_STATE
};

int startConnection(char **argv, Connection *server);
int requestFilename(char *filename, int bufSize, Connection *server);
void process_args(int argc, char **argv);

int main(int argc, char **argv) {
    int outFd = 0;
    STATE state = START_STATE;
    Connection server;
    server.sk_num = 0;
    process_args(argc, argv);
    sendErr_init(atof(argv[5]), DROP_ON, FLIP_ON, DEBUG_ON, RSEED_ON);

    while (state != DONE) {
        printf("state %d\n", state);
        switch(state) {
            case START_STATE:
                state = startConnection(argv, &server);
                break;
            case FILENAME:
                break;
            case FILE_OK:
                break;
            case RECV_DATA:
                break;
            case DONE:
                break;

            default:
                printf("ERROR - in default state\n");
                break;
        }
        exit(0);
    }

    return 0;
}

int startConnection(char **argv, Connection *server) {
    STATE returnValue = FILENAME;

    if (server->sk_num > 0) {
        close(server->sk_num);
    }

    if (udpClientSetup(argv[6], atoi(argv[7]), server) < 0) {
        printf("Bad\n");
        returnValue = DONE;
    } else {
        printf("Good\n");
        returnValue = FILENAME;
    }

    return returnValue;
}

int requestFilename(char *filename, int bufSize, Connection *server) {
    int returnValue = START_STATE;

    uint8_t packet[MAX_LEN];

    return returnValue;
}

void process_args(int argc, char **argv) {
    if (argc != 8) {
        printf("Usage %s fromFile toFile buffer_size window_size error_rate hostname port\n", argv[0]);
        exit(-1);
    } else if (strlen(argv[1]) > 100) {
        printf("fromFile name needs to be less than 100 characters and is %d\n", (int) strlen(argv[1]));
        exit(-1);
    } else if (strlen(argv[2]) > 100) {
        printf("toFile name needs to be less than 100 characters and is %d\n", (int) strlen(argv[2]));
        exit(-1);
    } else if (atoi(argv[4]) < 400 || atoi(argv[4]) > 1400) {
        printf("Buffer size needs to be between 400 and 1400 and is %s\n", argv[3]);
        exit(-1);
    } else if (atof(argv[5]) < 0 || atof(argv[5]) >= 1) {
        printf("Error rate needs to be between 0 and 1 and is %s\n", argv[4]);
        exit(-1);
    }
}
