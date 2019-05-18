#include "icmp.h"
#include "ip.h"
#include "rtable.h"
#include "arp.h"
#include "base.h"

#include <stdio.h>
#include <stdlib.h>

// send icmp packet
void icmp_send_packet(const char *in_pkt, int len, u8 type, u8 code, u32 sip)
{
	//fprintf(stderr, "TODO: malloc and send icmp packet.\n");
	int mlen;
	char* icmp_pkt;
	struct ether_header* eh_tmp = (struct ether_header*)in_pkt;
	struct iphdr* iph_tmp = (struct iphdr*)(in_pkt + ETHER_HDR_SIZE);
	struct icmphdr* icmp_tmp = (struct icmp*)(in_pkt + ETHER_HDR_SIZE + IP_HDR_SIZE(iph_tmp));

	if(type == 0 && code == 0)
	{
		icmp_pkt = (char *)malloc(len);
		struct ether_header* eh = (struct ether_header*)icmp_pkt;
		struct iphdr* iph = (struct iphdr*)(icmp_pkt + ETHER_HDR_SIZE);
		struct icmphdr* icmph = (struct icmphdr*)(icmp_pkt + ETHER_HDR_SIZE + IP_HDR_SIZE(iph_tmp));
		eh->ether_type = htons(ETH_P_IP);
		memcpy(eh->ether_shost, eh_tmp->ether_dhost, ETH_ALEN);
		memcpy(eh->ether_dhost, eh_tmp->ether_shost, ETH_ALEN);
		u32 daddr_tmp = sip;
		u32 saddr_tmp = ntohl(iph_tmp->saddr);
		ip_init_hdr(iph, daddr_tmp, saddr_tmp, len-ETHER_HDR_SIZE, IPPROTO_ICMP);
		icmp->type = 0;
		icmp->code = 0;

		memcpy( ((char *)icmph) + 4, 
					((char *)icmp_tmp) + 4, 
					len - ETHER_HDR_SIZE - IP_HDR_SIZE(iph_tmp));
		icmph->checksum = icmp_checksum(icmph, len - ETHER_HDR_SIZE - IP_HDR_SIZE(iph_tmp));
		mlen = len;
	}
	else
	{
		mlen = ETHER_HDR_SIZE + 2 * IP_HDR_SIZE(iph_tmp) + 16;
		icmp_pkt = (char *)malloc(mlen);
		struct ether_header* eh = (struct ether_header*)icmp_pkt;
		struct iphdr* iph = (struct iphdr*)(icmp_pkt + ETHER_HDR_SIZE);
		struct icmphdr* icmph = (struct icmphdr*)(icmp_pkt + ETHER_HDR_SIZE + IP_HDR_SIZE(iph_tmp));
		eh->ether_type = htons(ETH_P_IP);
		memcpy(eh->ether_shost, eh_tmp->ether_dhost, ETH_ALEN);
		memcpy( ((char *)icmph) + 8, 
					(char *)iph_tmp, 
					IP_HDR_SIZE(iph_tmp) + 8);
		u32 daddr_tmp = sip;
		u32 saddr_tmp = ntohl(iph_tmp->saddr);
		ip_init_hdr(iph, daddr_tmp, saddr_tmp, mlen-ETHER_HDR_SIZE, IPPROTO_ICMP);
		memset(((char *)icmph) + 4, 0, 4);
		icmph->type = type;
		icmph->code = code;
		icmp->checksum = icmp_checksum(icmph, mlen - ETHER_HDR_SIZE - IP_HDR_SIZE(iph_tmp));
	}
	ip_send_packet(icmp_pkt, mlen);
}
