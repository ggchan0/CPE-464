#include <stdio.h>
#include <pcap.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <netinet/ether.h>
#include <string.h>
#include <stdlib.h>

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

int main(int argc, char **argv) {
   pcap_t *p;
   struct pcap_pkthdr *pkt_header;
   const u_char *pkt_data;
   char errbuf[PCAP_ERRBUF_SIZE];
   int pcapNextStatus;

   if (!inputValid(argc, argv)) {
      exit(EXIT_FAILURE);
   } 

   p = pcap_open_offline(argv[1], errbuf);
   while ((pcapNextStatus = pcap_next_ex(p, &pkt_header, &pkt_data)) >= 0) {
      printf("Data: %d Caplen: %d Len: %d\n", pkt_data[1], pkt_header->caplen, pkt_header->len);
      
      pcapNextStatus = pcap_next_ex(p, &pkt_header, &pkt_data);
   }

   pcap_close(p);
   return 0;
}
