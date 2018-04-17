#include "trace.h"

int inputValid(int argc, char **argv) {
   char errbuf[PCAP_ERRBUF_SIZE];
   if (argc != 2) {
      fprintf(stderr, "Not enough arguments\n");
      return 0;
   } else {
      pcap_t *p = pcap_open_offline(argv[1], errbuf);
      if (p == NULL) {
         printf("%s\n", errbuf);
         return 0;
      }
   }

   return 1;
}

void readEtherHeader(etherHeader *e, const u_char *data) {

   memcpy(&(e->MACdest), data + ETH_MAC_DEST, MAC_SIZE);
   memcpy(&(e->MACsrc), data + ETH_MAC_SRC, MAC_SIZE);
   memcpy(&(e->type), data + ETH_TYPE, ETH_TYPE_SIZE);
}

char * getType(etherHeader e) {
   switch(e.type) {
      case(ARP):
         return "ARP";
         break;
      case(IP):
         return "IP";
         break;
      default:
         return "Unknown";
         break;
   }
}

void printEtherInformation(etherHeader *e, char *type) {
   printf("\tEthernet Header\n");
   printf("\t\tDest MAC: %s\n", ether_ntoa((const struct ether_addr *) e->MACdest));
   printf("\t\tSource MAC: %s\n", ether_ntoa((const struct ether_addr *) e->MACsrc));
   printf("\t\tType: %s\n\n", type);

}

void readARPHeader(arpHeader *arp, const u_char *data, int offset) {
   memcpy(&(arp->opcode), data + offset + ARP_OPCODE, ARP_OP_SIZE);
   memcpy(&(arp->senderMAC), data + offset + ARP_SRC_MAC, MAC_SIZE);
   memcpy(&(arp->destMAC), data + offset + ARP_DEST_MAC, MAC_SIZE);
   memcpy(&(arp->senderIP), data + offset + ARP_SRC_IP, IP_SIZE);
   memcpy(&(arp->destIP), data + offset + ARP_DEST_IP, IP_SIZE);
}

void printARPHeader(arpHeader *arp) {
   struct in_addr senderIP;
   struct in_addr targetIP;
   char *opcode;

   if (arp->opcode == ARP_REPLY) {
      opcode = "Reply";
   } else {
      opcode = "Request";
   }

   memcpy(&(senderIP.s_addr), arp->senderIP, IP_SIZE);
   memcpy(&(targetIP.s_addr), arp->destIP, IP_SIZE);

   printf("\tARP header\n");
   printf("\t\tOpcode: %s\n", opcode);
   printf("\t\tSender MAC: %s\n", ether_ntoa((const struct ether_addr *) arp->senderMAC));
   printf("\t\tSender IP: %s\n", inet_ntoa(senderIP));
   printf("\t\tTarget MAC: %s\n", ether_ntoa((const struct ether_addr *) arp->destMAC));
   printf("\t\tTarget IP: %s\n", inet_ntoa(targetIP));
   printf("\n");
}

void readIPHeader(ipHeader *ip, const u_char *data, int offset) {
   uint8_t ver_hdl = 0;
   uint8_t tos = 0;

   memcpy(&(ip->raw), data + offset, IP_HEADER_SIZE);
   memcpy(&ver_hdl, data + offset + IP_VER_HDL, 1);
   memcpy(&tos, data + offset + IP_TOS, 1);
   ip->version = (ver_hdl & 0xF0) >> 4;
   ip->headerLen = ver_hdl & 0x0F;
   ip->diffserv = (tos & 0xFC) >> 2;
   ip->ecn = tos & 0x3;
   memcpy(&(ip->totalLen), data + offset + IP_LEN, 2);
   ip->totalLen = ntohs(ip->totalLen);
   memcpy(&(ip->ttl), data + offset + IP_TTL_VAL, 1);
   memcpy(&(ip->protocol), data + offset + IP_PROTOCOL, 1);
   memcpy(&(ip->checksum), data + offset + IP_HDR_CHECKSUM, 2);
   ip->checksum = ntohs(ip->checksum);
   memcpy(&(ip->senderIP), data + offset + IP_SRC_ADDR, IP_SIZE);
   memcpy(&(ip->destIP), data + offset + IP_DEST_ADDR, IP_SIZE);
}

char * getIpProtocol(uint8_t protocol) {
   switch (protocol) {
      case(ICMP):
         return "ICMP";
         break;
      case(TCP):
         return "TCP";
         break;
      case(UDP):
         return "UDP";
         break;
      default:
         return "Unknown";
         break;
   }
}

void printIPHeader(ipHeader *ip) {
   struct in_addr senderIP;
   struct in_addr destIP;
   unsigned short int checksum;

   memcpy(&(senderIP.s_addr), ip->senderIP, IP_SIZE);
   memcpy(&(destIP.s_addr), ip->destIP, IP_SIZE);

   printf("\tIP Header\n");
   printf("\t\tIP Version: %u\n", ip->version);
   printf("\t\tHeader Len (bytes): %u\n", ip->headerLen * 4);
   printf("\t\tTOS subfields:\n");
   printf("\t\t\tDiffserv bits: %u\n", ip->diffserv);
   printf("\t\t\tECN bits: %u\n", ip->ecn);
   printf("\t\tTTL: %u\n", ip->ttl);
   printf("\t\tProtocol: %s\n", getIpProtocol(ip->protocol));

   checksum = in_cksum((unsigned short int *) ip->raw, IP_HEADER_SIZE);

   if (checksum == 0) {
      printf("\t\tChecksum: Correct (0x%04x)\n", ip->checksum);
   } else {
      printf("\t\tChecksum: Incorrect (0x%04x)\n", ip->checksum);
   }

   printf("\t\tSender IP: %s\n", inet_ntoa(senderIP));
   printf("\t\tDest IP: %s\n", inet_ntoa(destIP));
   printf("\n");
}

void readIcmpHeader(icmpHeader *icmp, const u_char *data, int offset) {
   memcpy(&(icmp->type), data + offset + ICMP_TYPE, 1);
}

void printIcmpHeader(icmpHeader *icmp) {
   printf("\tICMP Header\n");
   if (icmp->type == ICMP_REQUEST) {
      printf("\t\tType: Request\n");
   } else if (icmp->type == ICMP_REPLY){
      printf("\t\tType: Reply\n");
   } else {
      printf("\t\tType: %u\n", icmp->type);
   }

   printf("\n");
}

void readTcpHeader(tcpHeader *tcp, const u_char *data, int offset) {
   memcpy(&(tcp->sourcePort), data + offset + TCP_UDP_SRC_PORT, TCP_UDP_PORT_SIZE);
   tcp->sourcePort = ntohs(tcp->sourcePort);
   memcpy(&(tcp->destPort), data + offset + TCP_UDP_DEST_PORT, TCP_UDP_PORT_SIZE);
   tcp->destPort = ntohs(tcp->destPort);
   memcpy(&(tcp->seqNum), data + offset + TCP_SEQ_NUM, TCP_ORDER_SIZE);
   tcp->seqNum = ntohl(tcp->seqNum);
   memcpy(&(tcp->ackNum), data + offset + TCP_ACK_NUM, TCP_ORDER_SIZE);
   tcp->ackNum = ntohl(tcp->ackNum);
   memcpy(&(tcp->headerLen), data + offset + TCP_HEADER_LEN, 1);
   tcp->headerLen = (tcp->headerLen & 0xF0) >> 4;
   memcpy(&(tcp->flags), data + offset + TCP_FLAGS, 1);
   memcpy(&(tcp->windowSize), data + offset + TCP_WINDOW_LEN, 2);
   tcp->windowSize = ntohs(tcp->windowSize);
   memcpy(&(tcp->checksum), data + offset + TCP_CHECKSUM, 2);
   tcp->checksum = ntohs(tcp->checksum);
}

void printTcpFlags(uint8_t flags) {
   if (flags & TCP_SYN_BIT) {
      printf("\t\tSYN Flag: Yes\n");
   } else {
      printf("\t\tSYN Flag: No\n");
   }

   if (flags & TCP_RST_BIT) {
      printf("\t\tRST Flag: Yes\n");
   } else {
      printf("\t\tRST Flag: No\n");
   }

   if (flags & TCP_FIN_BIT) {
      printf("\t\tFIN Flag: Yes\n");
   } else {
      printf("\t\tFIN Flag: No\n");
   }

   if (flags & TCP_ACK_BIT) {
      printf("\t\tACK Flag: Yes\n");
   } else {
      printf("\t\tACK Flag: No\n");
   }
}

int calcTcpChecksum(tcpHeader *tcp, ipHeader *ip, const u_char *data, int offset) {
   uint8_t zero = 0;
   uint8_t tcpProtocol = 6;
   uint16_t tcpLen = ip->totalLen - IP_HEADER_SIZE;
   uint16_t tcpLenFormatted = htons(tcpLen);
   int dataSize = TCP_PSEUDO_SIZE + tcpLen;
   unsigned short checksumResult;

   void *datagram = malloc(dataSize);
   memcpy(datagram + TCP_PSEUDO_SRC, &(ip->senderIP), IP_SIZE);
   memcpy(datagram + TCP_PSEUDO_DEST, &(ip->destIP), IP_SIZE);
   memcpy(datagram + TCP_PSEUDO_RES, &zero, 1);
   memcpy(datagram + TCP_PSEUDO_PROTOCOL, &tcpProtocol, 1);
   memcpy(datagram + TCP_PSEUDO_TCP_LEN, &tcpLenFormatted, 2);
   memcpy(datagram + TCP_PSEUDO_TCP_DATA, data + offset, tcpLen);

   checksumResult = in_cksum(datagram, dataSize);
   free(datagram);
   return checksumResult;
}

void printTcpChecksum(tcpHeader *tcp, ipHeader *ip, const u_char *data, int offset) {

   if (calcTcpChecksum(tcp, ip, data, offset) == 0) {
      printf("\t\tChecksum: Correct (0x%04x)\n", tcp->checksum);
   } else {
      printf("\t\tChecksum: Incorrect (0x%04x)\n", tcp->checksum);
   }
}

void printTcpHeader(tcpHeader *tcp, ipHeader *ip, const u_char *data, int offset) {
   printf("\tTCP Header\n");
   if (tcp->sourcePort == TCP_PORT_HTTP) {
      printf("\t\tSource Port:  HTTP\n");
   } else {
      printf("\t\tSource Port:  %u\n", tcp->sourcePort);
   }

   if (tcp->destPort == TCP_PORT_HTTP) {
      printf("\t\tDest Port:  HTTP\n");
   } else {
      printf("\t\tDest Port:  %u\n", tcp->destPort);
   }
   printf("\t\tSequence Number: %u\n", (unsigned int) tcp->seqNum);
   printf("\t\tACK Number: %u\n", (unsigned int) tcp->ackNum);
   printf("\t\tData Offset (bytes): %u\n", tcp->headerLen * 4);
   printTcpFlags(tcp->flags);
   printf("\t\tWindow Size: %u\n", tcp->windowSize);
   printTcpChecksum(tcp, ip, data, offset);
}

void readUdpHeader(udpHeader *udp, const u_char *data, int offset) {
   memcpy(&(udp->sourcePort), data + offset + TCP_UDP_SRC_PORT, TCP_UDP_PORT_SIZE);
   udp->sourcePort = ntohs(udp->sourcePort);
   memcpy(&(udp->destPort), data + offset + TCP_UDP_DEST_PORT, TCP_UDP_PORT_SIZE);
   udp->destPort = ntohs(udp->destPort);
}

void printUdpHeader(udpHeader *udp) {
   printf("\tUDP Header\n");
   if (udp->sourcePort == UDP_PORT_DNS) {
      printf("\t\tSource Port:  DNS\n");
   } else {
      printf("\t\tSource Port:  %u\n", udp->sourcePort);
   }

   if (udp->destPort == UDP_PORT_DNS) {
      printf("\t\tDest Port:  DNS\n");
   } else {
      printf("\t\tDest Port:  %u\n", udp->destPort);
   }
}

void processIpPacket(const u_char *pkt_data, int offset, ipHeader *ip) {
   icmpHeader icmp;
   tcpHeader tcp;
   udpHeader udp;

   switch(ip->protocol) {
      case(ICMP):
         readIcmpHeader(&icmp, pkt_data, offset);
         printIcmpHeader(&icmp);
         break;
      case(TCP):
         readTcpHeader(&tcp, pkt_data, offset);
         printTcpHeader(&tcp, ip, pkt_data, offset);
         break;
      case(UDP):
         readUdpHeader(&udp, pkt_data, offset);
         printUdpHeader(&udp);
         break;
      default:
         break;
   }
}

void processPackets(pcap_t *p) {
   int packetNumber = 0;
   struct pcap_pkthdr *pkt_header;
   const u_char *pkt_data;

   while (pcap_next_ex(p, &pkt_header, &pkt_data) != PCAP_END) {
      int offset = 0;
      etherHeader eth;
      char *packetType;

      packetNumber++;
      printf("Packet number: %u  Packet Len: %u\n\n", packetNumber, pkt_header->caplen);
      readEtherHeader(&eth, pkt_data);
      packetType = getType(eth);
      printEtherInformation(&eth, packetType);

      offset += ETH_HEADER_SIZE;
      if (strcmp(packetType, "ARP") == 0) {
         arpHeader arp;
         readARPHeader(&arp, pkt_data, offset);
         printARPHeader(&arp);
      } else if (strcmp(packetType, "IP") == 0) {
         ipHeader ip;
         readIPHeader(&ip, pkt_data, offset);
         printIPHeader(&ip);

         offset += (ip.headerLen * 4);

         processIpPacket(pkt_data, offset, &ip);
      }

      offset += ETH_HEADER_SIZE;
      printf("\n");
   }
}

int main(int argc, char **argv) {
   pcap_t *p;
   char errbuf[PCAP_ERRBUF_SIZE];


   if (!inputValid(argc, argv)) {
      exit(EXIT_FAILURE);
   }

   p = pcap_open_offline(argv[1], errbuf);
   printf("\n");
   processPackets(p);

   pcap_close(p);
   return 0;
}
