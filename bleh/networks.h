
// 	Writen - HMS April 2017
//  Supports TCP and UDP - both client and server
//  Code copied from Professor Hugh Smith's Stop and Wait design


#ifndef __NETWORKS_H__
#define __NETWORKS_H__

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#define BACKLOG 10
#define MAX_LEN 1500

#define HEADER_SIZE 7
#define CRC_ERROR -1

#define START_SEQ_NUM 1

#define MAX_TRIES 10
#define LONG_TIME 10
#define SHORT_TIME 1

#define SEQ_NUM_OFFSET 0
#define SEQ_NUM_SIZE 4
#define CHECK_SUM_OFFSET 4
#define CHECK_SUM_SIZE 2
#define FLAG_OFFSET 6
#define FLAG_SIZE 1

/* File Name Exchange */
#define WINDOW_LEN_OFFSET 0
#define WINDOW_LEN_SIZE 4
#define BUFFER_LEN_OFFSET 4
#define BUFFER_LEN_SIZE 4
#define FILENAME_LEN_OFFSET 8
#define FILENAME_LEN_SIZE 1
#define FILENAME_OFFSET 9

enum FLAG {
   ZERO, SETUP_REQ, SETUP_RES, DATA, UNUSED, RR, SREJ, FILENAME_REQ, FILENAME_RES, FILENAME_ERR, END_OF_FILE, END
};

enum SELECT {
   SET_NULL, NOT_NULL
};

typedef struct connection {
    int sk_num;
    struct sockaddr_in6 remote;
    uint32_t len;
} __attribute__((packed)) Connection;

typedef struct header {
   uint32_t seq_num;
   uint16_t checksum;
   uint8_t flag;
} __attribute__((packed)) Header;

/*
typedef struct windowBuf {
    uint32_t buf_size;
    uint32_t seq_num;
    uint8_t *buffer;
} windowBuf;
*/

typedef struct windowBuf {
   uint32_t buf_size;
   uint32_t seq_num;
   uint8_t buffer[MAX_LEN];
} windowBuf;

typedef struct window {
   uint32_t bottom;
   uint32_t middle;
   uint32_t top;
   uint32_t size;

   windowBuf *buf;
   uint8_t *isValid;
} Window;


int safeSend(uint8_t *packet, uint32_t len, Connection *connection);
int safeRecv(int sk_num, char *data_buf, int len, Connection *connection);
int sendBuf(uint8_t *buf, uint32_t len, Connection *connection, uint8_t flag, uint32_t seq_num, uint8_t * packet);
int recv_buf(uint8_t *buf, int len, int sk_num, Connection *connection, uint8_t *flag, int *seq_num);
int createHeader(uint32_t len, uint8_t flag, uint32_t seq_num, uint8_t *packet);
int retrieveHeader(char *data_buf, int recv_len, uint8_t *flag, uint32_t *seq_num);
void insertIntoWindow(Window *window, uint8_t *packet, int packetLen, int seq_num);
void loadFromWindow(Window *window, uint8_t *packet, uint32_t *len_read, int seq_num);
void removeFromWindow(Window *window, int seq_num);
void slideWindow(Window *window, int new_bottom);
void freeWindow(Window *window);
void initWindow(Window *window, int buf_size, int window_size);
void *checked_calloc(size_t size);

int processSelect(Connection *client, int *retryCount, int selectTimeoutState, int dataReadyState, int doneState);

int select_call(int sk_num, int sec, int microsec, int set_null);

// for the server side
int udpServerSetup(int portNumber);

// for the client side
int udpClientSetup(char *hostname, int portNumber, Connection *connection);

#endif
