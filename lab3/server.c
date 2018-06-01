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
    START, DONE, FILENAME, ACK_CLIENT, RECV_DATA
};


void processServer(int sk_num);
void processClient(int sk_num, uint8_t *buf, int recv_len, Connection *client);
STATE filename(Connection *client, uint8_t *buf, int recv_len, int *data_file, uint32_t *buf_size, uint32_t *window_size, Window *window);
STATE ackClient(Connection *client, uint8_t *buf, int *data_file);
STATE recv_data(Connection *client, uint8_t *buf, int *data_file, int *data_received, Window *window, int *retryCount, int *expected_seq_num);
int processArgs(int argc, char **argv);

int main(int argc, char **argv) {
    int sk_num = 0;
    int portNumber = 0;

    portNumber = processArgs(argc, argv);
    sendtoErr_init(atof(argv[1]), DROP_ON, FLIP_ON, DEBUG_OFF, RSEED_ON);

    sk_num = udpServerSetup(portNumber);

    processServer(sk_num);

    return 0;
}

void processServer(int sk_num) {
    pid_t pid = 0;
    int status = 0;
    uint8_t buf[MAX_LEN];
    Connection client;
    uint8_t flag = 0;
    int seq_num = 0;
    int recv_len = 0;

    while(1) {
        if (select_call(sk_num, LONG_TIME, 0, SET_NULL) == 1) {
            recv_len = recv_buf(buf, MAX_LEN, sk_num, &client, &flag, &seq_num);
            printf("recv %d\n", recv_len);
            if (recv_len != CRC_ERROR) {
                printf("forked!\n");
                if ((pid = fork()) < 0) {
                    perror("fork");
                    exit(-1);
                }

                if (pid == 0) {
                    processClient(sk_num, buf, recv_len, &client);
                    exit(0);
                }
            }

            while (waitpid(-1, &status, WNOHANG) > 0) {
                /*empty while loop */
            }
        }
    }
}

void processClient(int sk_num, uint8_t *buf, int recv_len, Connection *client) {
    STATE state = START;
    int data_file = 0;
    int packet_len = 0;
    uint8_t packet[MAX_LEN];
    uint32_t buf_size = 0;
    uint32_t window_size = 0;
    uint32_t seq_num = START_SEQ_NUM;
    int data_received = 0;
    static int retryCount = 0;
    Window window;

    client->remote.sin6_family = AF_INET6;

    while (state != DONE) {
        switch(state) {
            case START:
                state = FILENAME;
                break;
            case FILENAME:
                state = filename(client, buf, recv_len, &data_file, &buf_size, &window_size, &window);
                break;
            case ACK_CLIENT:
                state = ackClient(client, buf, &data_file);
            case RECV_DATA:
                state = recv_data(client, buf, &data_file, &data_received, &window, &retryCount, &seq_num);
                break;
            default:
                state = DONE;
                break;
        }
    }

    printf("Child exiting\n");
    exit(0);
}

STATE filename(Connection *client, uint8_t *buf, int recv_len, int *data_file, uint32_t *buf_size, uint32_t *window_size, Window *window) {
    uint8_t response = 0;
    char fname[MAX_LEN];
    STATE returnValue = DONE;
    uint8_t filenameLen;

    memcpy(window_size, buf + WINDOW_LEN_OFFSET, WINDOW_LEN_SIZE);
    *window_size = ntohl(*window_size);
    memcpy(buf_size, buf + BUFFER_LEN_OFFSET, BUFFER_LEN_SIZE);
    *buf_size = ntohl(*buf_size);
    memcpy(&filenameLen, buf + FILENAME_LEN_OFFSET, FILENAME_LEN_SIZE);
    memcpy(fname, buf + FILENAME_OFFSET, filenameLen);

    fname[filenameLen] = '\0';

    if ((client->sk_num = socket(AF_INET6, SOCK_DGRAM, 0)) < 0) {
        perror("filename, open client socket");
        exit(-1);
    }

    *data_file = open(fname, O_CREAT | O_TRUNC | O_WRONLY, 0600);

    initWindow(window, *buf_size, *window_size);

    returnValue = ACK_CLIENT;

    return returnValue;
}

STATE ackClient(Connection *client, uint8_t *buf, int *data_file) {
    uint8_t response = 0;
    int returnValue = 0;

    printf("sending ack to client\n");

    if (*data_file > 0) {
        sendBuf(&response, 0, client, FILENAME_RES, 0, buf);
        returnValue = RECV_DATA;
    } else if (*data_file < 0) {
        sendBuf(&response, 0, client, FILENAME_ERR, 0, buf);
        returnValue = DONE;
    }

    return returnValue;
}

STATE recv_data(Connection *client, uint8_t *buf, int *data_file, int *data_received, Window *window, int *retryCount, int *expected_seq_num) {
    int timeoutFlag = RECV_DATA;
    int returnValue = RECV_DATA;
    uint8_t flag = 0;
    uint32_t seq_num = 0;
    uint8_t packet[MAX_LEN];
    uint32_t data_len = 0;

    if (*data_received = 0) {
        returnValue = processSelect(client, retryCount, ACK_CLIENT, RECV_DATA, DONE);
        if (returnValue != RECV_DATA) {
            return returnValue;
        }
    } else if (select_call(client->sk_num, LONG_TIME, 0, NOT_NULL) == 0) {
        printf("No data from client in %d seconds, shutting down\n", LONG_TIME);
        return DONE;
    }
    

    *data_received = 1;

    data_len = recv_buf(packet, MAX_LEN, client->sk_num, client, &flag, &seq_num);

    printf("Got something!\n");

    /*
    if (data_len == CRC_ERROR) {
        return RECV_DATA;
    }

    if (flag == EOF) {
        send_buf(packet, 0, client, EOF, 0, packet);
        printf("File done\n");
        return WAIT_CLIENT_END;
    } else {
        send_buf(packet, )
    }
    */

    
    return returnValue;
}

int processArgs(int argc, char **argv) {
    int portNumber = 0;
    double errRate = 0.0;

    if (argc < 2 || argc > 3) {
        printf("Usage %s error_rate [port_number]\n", argv[0]);
        exit(-1);
    }

    errRate = atof(argv[1]);
    if (errRate < 0 || errRate > 1) {
        printf("Error rate must be between 0 and 1, and is %.2f\n", errRate);
        exit(-1);
    }

    if (argc == 3) {
        portNumber = atoi(argv[2]);
    } else {
        portNumber = 0;
    }

    return portNumber;
}
