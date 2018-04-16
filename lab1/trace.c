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
   printf("type int: %d\n", e.type);
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
   printf("\t\tType: %s\n", type);

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
}

void readIPHeader(ipHeader * ip, const u_char pkt_data, int offset) {
   
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
      printf("Packet number: %d  Packet Len: %d\n\n", packetNumber, pkt_header->caplen);
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
   processPackets(p);

   pcap_close(p);
   return 0;
}
