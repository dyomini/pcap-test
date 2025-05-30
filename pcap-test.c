#include <pcap.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <libnet.h>

void usage() {
	printf("syntax: pcap-test <interface>\n");
	printf("sample: pcap-test wlan0\n");
}

void printEth(struct libnet_ethernet_hdr* ethHdr){
	printf("src MAC address of Ethernet Header: ");
	for(int i=5;i>0;i--){
		printf("%02x:",((char*)ethHdr->ether_shost)[i]);
	}
	printf("%02x\n",((char*) ethHdr->ether_shost)[0]);
	printf("dst MAC address of Ethernet Header: ");
	for(int i=5;i>0;i--){
		printf("%02x:" ,((char*) ethHdr->ether_dhost)[i]);
	}
	printf("%02x\n\n",((char*)ethHdr->ether_dhost)[0]);
}

void printIpv4(struct libnet_ipv4_hdr* ipv4Hdr){
	printf("src IP of IP Header: %s\n",inet_ntoa(ipv4Hdr->ip_src));
	printf("dst IP of IP Header: %s\n\n", inet_ntoa(ipv4Hdr->ip_dst));
}

void printTcp(struct libnet_tcp_hdr* tcpHdr){
	printf("src PORT of TCP: %d\n",ntohs(tcpHdr->th_sport));
	printf("dst PORT of TCP: %d\n\n",ntohs(tcpHdr->th_dport));
}

void printPayload(u_char* data,size_t data_len){
	printf("Payload(Data):\n");
	for (int i=0;i<data_len;i++){
		printf("%02x ",*(data+i));
	}
	printf("\n\n#######################\n\n");
}

typedef struct {
	char* dev_;
} Param;

Param param = {
	.dev_ = NULL
};
bool parse(Param* param, int argc, char* argv[]) {
	if (argc != 2) {
		usage();
		return -1;
	}
	param->dev_ = argv[1];
	return true;
}

int main(int argc, char* argv[]) {
	if (!parse(&param, argc, argv))
		return -1;

	char errbuf[PCAP_ERRBUF_SIZE];
	pcap_t* pcap = pcap_open_live(param.dev_, BUFSIZ, 1, 1000, errbuf);
	if (pcap == NULL) {
		fprintf(stderr, "pcap_open_live(%s) return null - %s\n", param.dev_, errbuf);
		return -1;
	}

	while (true) {
		struct pcap_pkthdr* header;
		const u_char* packet;
		u_char* p;

		struct libnet_ethernet_hdr* ethHdr;
		struct libnet_ipv4_hdr* ipv4Hdr;
		struct libnet_tcp_hdr* tcpHdr;

		int res = pcap_next_ex(pcap, &header, &packet);
		if (res == 0) continue;
		if (res == PCAP_ERROR || res == PCAP_ERROR_BREAK) {
			printf("pcap_next_ex return %d(%s)\n", res, pcap_geterr(pcap));
			break;
		}
		
		p=(u_char*) packet;
		ethHdr = (struct libnet_ethernet_hdr*)p;
		ipv4Hdr = (char*)ethHdr + sizeof(*ethHdr); 
		tcpHdr = (char*)ipv4Hdr+(ipv4Hdr->ip_hl)*4;
		uint16_t ethType=ntohs(ethHdr->ether_type);
		uint8_t ipv4Protocol=ipv4Hdr->ip_p;
		
		if(ethType==ETHERTYPE_IP){	
			if(ipv4Protocol==IPPROTO_TCP){
				printf("%u bytes captured\n", header->caplen);
				
				printEth((u_char*) p);
				p+=sizeof(ethHdr);
				printIpv4((u_char*) p);
				p=((char*)ipv4Hdr+(ipv4Hdr->ip_hl)*4);
				printTcp((u_char*) p);
				char *data = (char*)tcpHdr+(tcpHdr->th_off*4);
				uint32_t data_len = (header->caplen) - (data - (char*)packet);
				printPayload(data, data_len>=20?20:data_len);
			}
		}
	}
	pcap_close(pcap);
}