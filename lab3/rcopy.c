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
    DONE, FILENAME, SEND_DATA, START_STATE, PROCESS_SERVER_RESPONSE, WAIT_ON_ACK, EXIT
};

int f;
char sbuf[10];

int startConnection(char **argv, Connection *server);
int sendFilename(char *filename, int bufSize, int windowSize, Connection *server);
STATE sendData(Connection *server, int data_file, int *seq_num, Window *window, int buf_size, int *last_seq_num);
STATE process(Connection *server, Window *window, int *last_seq_num);
STATE wait_on_ack(Connection *server, Window *window, int *retryCount, int *last_seq_num);
STATE exit_rcopy(Connection *server, int *retryCount);
void process_args(int argc, char **argv);

int main(int argc, char **argv) {
    int outFd = 0;
    int data_file = 0;
    int expected_seq_num = START_SEQ_NUM;
    STATE state = START_STATE;
    Connection server;
    Window window;
    static int retryCount = 0;
    server.sk_num = 0;
    process_args(argc, argv);
    data_file = open(argv[1], O_RDONLY);
    sendErr_init(atof(argv[5]), DROP_ON, FLIP_ON, DEBUG_ON, RSEED_ON);
    initWindow(&window, atoi(argv[4]), atoi(argv[3]));
    int last_seq_num = 0;
    f = open("something.txt", O_CREAT | O_TRUNC | O_RDWR, 0644);

    while (state != DONE) {
        switch(state) {
            case START_STATE:
                state = startConnection(argv, &server);
                break;
            case FILENAME:
                state = sendFilename(argv[2], atoi(argv[4]), atoi(argv[3]), &server);
                break;
            case SEND_DATA:
                state = sendData(&server, data_file, &expected_seq_num, &window, atoi(argv[4]), &last_seq_num);
                break;
            case PROCESS_SERVER_RESPONSE:
                state = process(&server, &window, &last_seq_num);
                break;
            case WAIT_ON_ACK:
                state = wait_on_ack(&server, &window, &retryCount, &last_seq_num);
                break;
            case EXIT:
                state = exit_rcopy(&server, &retryCount);
                break;
            case DONE:
                freeWindow(&window);
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
        returnValue = DONE;
    } else {
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
        if (recv_check == CRC_ERROR) {
            returnValue = FILENAME;
        } else if (flag == FILENAME_ERR) {
            printf("Error during file open of %s on server\n", filename);
            returnValue = DONE;
        } else if (flag == FILENAME_RES) {
            returnValue = SEND_DATA;
        }
    }

    return returnValue;
}

STATE sendData(Connection *server, int data_file, int *seq_num, Window *window, int buf_size, int *last_seq_num) {
    uint8_t buf[MAX_LEN];
    uint8_t packet[MAX_LEN];
    uint32_t len_read = 0;
    uint32_t packet_len = 0;
    int returnValue = DONE;

    if (window->middle == window->top) {
        return WAIT_ON_ACK;
    }

    len_read = read(data_file, buf, buf_size);
    snprintf(sbuf, 10, "\nw %d\n", *seq_num);
    write(f, sbuf, strlen(sbuf));
    write(f, buf, len_read);

    switch(len_read) {
        case -1:
            perror("rcopy sendData, read error");
            returnValue = DONE;
            break;
        case 0:
            *last_seq_num = *seq_num;
            returnValue = WAIT_ON_ACK;
            break;
        default:
            insertIntoWindow(window, buf, len_read, *seq_num);
            window->middle++;
            sendBuf(buf, len_read, server, DATA, *seq_num, packet);
            (*seq_num)++;
            if (len_read < buf_size) {
                *last_seq_num =  *seq_num;
            }
            returnValue = PROCESS_SERVER_RESPONSE;
            break;
    }

    return returnValue;
}

STATE process(Connection *server, Window *window, int *last_seq_num) {
    int len_read = 0;
    int buf_size = 0;
    uint8_t buf[MAX_LEN];
    uint8_t packet[MAX_LEN];
    uint8_t flag = 0;
    uint32_t seq_num = 0;

    if (select_call(server->sk_num, 0, 0, NOT_NULL) == 0) {
        return SEND_DATA;
    }

    len_read = recv_buf(buf, MAX_LEN, server->sk_num, server, &flag, &seq_num);

    if (len_read == CRC_ERROR) {
        return WAIT_ON_ACK;
    }

    if (flag == RR) {
        if (seq_num == *last_seq_num) {
            return EXIT;
        } else if (seq_num >= window->bottom) {
            slideWindow(window, seq_num);
        }
    } else if (flag == SREJ) {
        loadFromWindow(window, buf, &buf_size, seq_num);

        sendBuf(buf, buf_size, server, DATA, seq_num, packet);
    }

    return PROCESS_SERVER_RESPONSE;
}

STATE wait_on_ack(Connection *server, Window *window, int *retryCount, int *last_seq_num) {
    uint32_t len_read = 0;
    uint32_t buf_size = 0;
    uint8_t buf[MAX_LEN];
    uint8_t packet[MAX_LEN];
    uint8_t flag = 0;

    if (*retryCount > MAX_TRIES) {
        printf("Sent packet %d times, quitting\n", MAX_TRIES);
        return DONE;
    }

    if (select_call(server->sk_num, SHORT_TIME, 0, NOT_NULL) == 0) {
        (*retryCount) += 1;

        if (*last_seq_num == window->bottom) {
            return WAIT_ON_ACK;
        }

        loadFromWindow(window, buf, &buf_size, window->bottom);
        sendBuf(buf, buf_size, server, DATA, window->bottom, packet);

        return WAIT_ON_ACK;
    }

    *retryCount = 0;
    return PROCESS_SERVER_RESPONSE;
}

STATE exit_rcopy(Connection *server, int *retryCount) {
    uint8_t response = 0;
    uint8_t packet[MAX_LEN];

    if (*retryCount > MAX_TRIES) {
        printf("Sent exit packet %d times, quitting\n", MAX_TRIES);
        return DONE;
    }

    sendBuf(&response, 0, server, END_OF_FILE, 0, packet);

    if (select_call(server->sk_num, SHORT_TIME, 0, NOT_NULL) == 0) {
        (*retryCount)++;
        return EXIT;
    }

    sendBuf(&response, 0, server, END, 0, packet);

    return DONE;
}

void process_args(int argc, char **argv) {
    if (argc != 8) {
        printf("Usage %s fromFile toFile window_size buffer_size error_rate hostname port\n", argv[0]);
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
    } else if (atoi(argv[4]) < 400 || atoi(argv[4]) > 1400) {
        printf("Buffer size needs to be between 400 and 1400 and is %s\n", argv[4]);
        exit(-1);
    } else if (atoi(argv[3]) <= 0) {
        printf("Window size needs to be a positive integer and is %s\n", argv[3]);
        exit(-1);
    } else if (atof(argv[5]) < 0 || atof(argv[5]) >= 1) {
        printf("Error rate needs to be between 0 and 1 and is %s\n", argv[5]);
        exit(-1);
    }
}
