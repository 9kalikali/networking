#include "ip.h"
#include "icmp.h"
#include "packet.h"
#include "arpcache.h"
#include "rtable.h"
#include "arp.h"
#include "log.h"

#include <stdio.h>
#include <stdlib.h>

// initialize ip header 
void ip_init_hdr(struct iphdr *ip, u32 saddr, u32 daddr, u16 len, u8 proto)
{
	ip->version = 4;
	ip->ihl = 5;
	ip->tos = 0;
	ip->tot_len = htons(len);
	ip->id = rand();
	ip->frag_off = htons(IP_DF);
	ip->ttl = DEFAULT_TTL;
	ip->protocol = proto;
	ip->saddr = htonl(saddr);
	ip->daddr = htonl(daddr);
	ip->checksum = ip_checksum(ip);
}

// lookup in the routing table, to find the entry with the same and longest prefix.
// the input address is in host byte order
rt_entry_t *longest_prefix_match(u32 dst)
{
	//fprintf(stderr, "TODO: longest prefix match for the packet.\n");
	rt_entry_t* entry = NULL;
	rt_entry_t* res = NULL;
	u32 num = 0;
	list_for_each_entry(entry, &rtable, list)
	{
		u32 net = entry->dest & entry->mask;
		u32 net_d = dst & entry->mask;
		if(net_d == net && entry->mask > num)
		{
			num = entry->mask;
			res = entry;
		}
	} 
	return res;
}

// send IP packet
//
// Different from ip_forward_packet, ip_send_packet sends packet generated by
// router itself. This function is used to send ICMP packets.
void ip_send_packet(char *packet, int len)
{
	//fprintf(stderr, "TODO: send ip packet.\n");
	struct iphdr* ip = packet_to_ip_hdr(packet);
	u32 dst = ntohl(ip->daddr);
	rt_entry_t *entry = longest_prefix_match(dst);
	if( !entry )
	{
		log(ERROR, "Cannot find forwarding rule for IP (destiantion:"IP_FMT") packet", 
				HOST_IP_FMT_STR(dst));
		free(packet);
		return;
	}

	u32 next_hop = entry->gw;
	if(!next_hop) next_hop = dst;

	struct ether_header* eh = (struct ether_header*)(packet);
	eh->ether_type = htons(ETH_P_IP);
	struct iphdr* iph;
	iph = (struct iphdr*)(packet + ETHER_HDR_SIZE);
	iph->saddr = htonl(entry->iface->ip);
	memcpy(eh->ether_shost, entry->iface->mac, ETH_ALEN);
	iface_send_packet_by_arp(entry->iface, next_hop, packet, len);
}
