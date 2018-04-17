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
#define IP_TTL_VAL 8
#define IP_PROTOCOL 9
#define IP_HDR_CHECKSUM 10
#define IP_SRC_ADDR 12
#define IP_DEST_ADDR 16

#define ICMP_TYPE 0
#define ICMP_REQUEST 8
#define ICMP_REPLY 0

#define TCP_UDP_SRC_PORT 0
#define TCP_UDP_DEST_PORT 2
#define TCP_UDP_PORT_SIZE 2
#define TCP_SEQ_NUM 4
#define TCP_ACK_NUM 8
#define TCP_ORDER_SIZE 4
#define TCP_HEADER_LEN 12
#define TCP_FLAGS 13
#define TCP_WINDOW_LEN 14
#define TCP_CHECKSUM 16
#define TCP_PORT_HTTP 80

#define TCP_PSEUDO_SIZE 12
#define TCP_PSEUDO_SRC 0
#define TCP_PSEUDO_DEST 4
#define TCP_PSEUDO_RES 8
#define TCP_PSEUDO_PROTOCOL 9
#define TCP_PSEUDO_TCP_LEN 10
#define TCP_PSEUDO_TCP_DATA 12

#define TCP_SYN_BIT 2
#define TCP_RST_BIT 4
#define TCP_FIN_BIT 1
#define TCP_ACK_BIT 16

#define UDP_PORT_DNS 53

#define ICMP 1
#define TCP 6
#define UDP 17

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
    uint8_t raw[IP_HEADER_SIZE];
    uint8_t version;
    uint8_t headerLen;
    uint16_t totalLen;
    uint8_t diffserv;
    uint8_t ecn;
    uint8_t ttl;
    uint8_t protocol;
    uint16_t checksum;
    uint8_t senderIP[IP_SIZE];
    uint8_t destIP[IP_SIZE];
} __attribute__((packed)) ipHeader;

typedef struct icmpHeader {
    uint8_t type;
} __attribute__((packed)) icmpHeader;

typedef struct tcpHeader {
    uint16_t sourcePort;
    uint16_t destPort;
    uint32_t seqNum;
    uint32_t ackNum;
    uint8_t headerLen;
    uint16_t windowSize;
    uint8_t flags;
    uint16_t checksum;
} __attribute__((packed)) tcpHeader;

typedef struct udpHeader {
    uint16_t sourcePort;
    uint16_t destPort;
} __attribute__((packed)) udpHeader;


int inputValid(int argc, char **argv);

void readEtherHeader(etherHeader *e, const u_char *data);

void processPackets(pcap_t *p);
