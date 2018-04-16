#include <stdio.h>
#include <pcap.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <netinet/ether.h>
#include <string.h>
#include <stdlib.h>
#include "checksum.h"

#define IP_SIZE 4
#define MAC_SIZE 6

#define ARP 1544
#define IP 8

#define PCAP_END -2

#define ETH_HEADER_SIZE 14
#define ETH_MAC_DEST 0
#define ETH_MAC_SRC 6
#define ETH_TYPE 12
#define ETH_TYPE_SIZE 2

#define ARP_HEADER_SIZE 28
#define ARP_OPCODE 6
#define ARP_SRC_MAC 8
#define ARP_SRC_IP 14
#define ARP_DEST_MAC 18
#define ARP_DEST_IP 24
#define ARP_OP_SIZE 2
#define ARP_REPLY 512
#define ARP_RESPONSE 256

#define IP_HEADER_SIZE 20
#define IP_VER_HDL 0
#define IP_TOS 1
#define IP_LEN 2
#define IP_TTL 8
#define IP_PROTOCOL 9
#define IP_HDR_CHECKSUM 10
#define IP_SRC_ADDR 12
#define IP_DEST_ADDR 16

typedef struct etherHeader {
    uint8_t MACdest[MAC_SIZE];
    uint8_t MACsrc[MAC_SIZE];
    uint16_t type;
} __attribute__((packed)) etherHeader;

typedef struct arpHeader {
    uint16_t opcode;
    uint8_t senderMAC[MAC_SIZE];
    uint8_t destMAC[MAC_SIZE];
    uint8_t senderIP[IP_SIZE];
    uint8_t destIP[IP_SIZE];
} __attribute__((packed)) arpHeader;

typedef struct ipHeader {
    uint8_t version;
    uint8_t headerLen;
    uint8_t diffserv;
    uint8_t ecn;
    uint8_t ttl;
    uint8_t protocol;
    uint16_t checksum;
    uint8_t senderIP[IP_SIZE];
    uint8_t destIP[IP_SIZE];
} __attribute__((packed)) ipHeader;

int inputValid(int argc, char **argv);

void readEtherHeader(etherHeader *e, const u_char *data);

void processPackets(pcap_t *p);
