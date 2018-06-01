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
    DONE, FILENAME, SEND_DATA, START_STATE, WAIT_ON_ACK, TIMEOUT_ON_ACK, WAIT_ON_EOF_ACK, TIMEOUT_ON_EOF_ACK
};

int startConnection(char **argv, Connection *server);
int sendFilename(char *filename, int bufSize, int windowSize, Connection *server);
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
                state = sendFilename(argv[2], atoi(argv[3]), atoi(argv[4]), &server);
                break;
            case SEND_DATA:
                break;
            case DONE:
                break;

            default:
                printf("ERROR - in default state\n");
                break;
        }
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

int sendFilename(char *filename, int bufSize, int windowSize, Connection *server) {
    int returnValue = START_STATE;
    uint32_t nlWindowSize = htonl(windowSize);
    uint32_t nlBufSize = htonl(bufSize);
    uint32_t recv_check = 0;
    uint8_t flag = 0;
    uint32_t seq_num = 0;
    uint8_t filenameLen = strlen(filename);
    uint8_t packet[MAX_LEN];
    uint8_t buf[MAX_LEN];
    int payloadLen = WINDOW_LEN_SIZE + BUFFER_LEN_SIZE + FILENAME_LEN_SIZE + strlen(filename);
    static int retryCount = 0;
    int offset = 0;

    printf("payload len %d\n", payloadLen);
    memcpy(buf + offset, &nlWindowSize, WINDOW_LEN_SIZE);
    offset += WINDOW_LEN_SIZE;
    memcpy(buf + offset, &nlBufSize, BUFFER_LEN_SIZE);
    offset += BUFFER_LEN_SIZE;
    memcpy(buf + offset, &filenameLen, FILENAME_LEN_SIZE);
    offset += FILENAME_LEN_SIZE;
    memcpy(buf + offset, filename, filenameLen);

    sendBuf(buf, payloadLen, server, FILENAME_REQ, 0, packet);

    if ((returnValue = processSelect(server, &retryCount, FILENAME, SEND_DATA, DONE)) == SEND_DATA) {
        recv_check = recv_buf(packet, MAX_LEN, server->sk_num, server, &flag, &seq_num);
        printf("here3\n");
        if (recv_check == CRC_ERROR) {
            returnValue = FILENAME;
        }
    }

    return returnValue;
}

void process_args(int argc, char **argv) {
    if (argc != 8) {
        printf("Usage %s fromFile toFile buffer_size window_size error_rate hostname port\n", argv[0]);
        exit(-1);
    } else if (strlen(argv[1]) > 100) {
        printf("fromFile name needs to be less than 100 characters and is %d\n", (int) strlen(argv[1]));
        exit(-1);
    } else if (access(argv[1], F_OK) == -1) {
        printf("Error opening file: %s does not exist\n", argv[1]);
        exit(-1);
    } else if (access(argv[1], F_OK) == -1) {
        printf("Error opening file: %s bad permissions\n", argv[1]);
        exit(-1);
    } else if (strlen(argv[2]) > 100) {
        printf("toFile name needs to be less than 100 characters and is %d\n", (int) strlen(argv[2]));
        exit(-1);
    } else if (atoi(argv[3]) < 400 || atoi(argv[3]) > 1400) {
        printf("Buffer size needs to be between 400 and 1400 and is %s\n", argv[3]);
        exit(-1);
    } else if (atoi(argv[4]) <= 0) {
        printf("Window size needs to be a positive integer and is %s\n", argv[4]);
        exit(-1);
    } else if (atof(argv[5]) < 0 || atof(argv[5]) >= 1) {
        printf("Error rate needs to be between 0 and 1 and is %s\n", argv[4]);
        exit(-1);
    }
}
