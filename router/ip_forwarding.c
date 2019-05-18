#include "ip.h"
#include "icmp.h"
#include "rtable.h"
#include "arp.h"
#include "arpcache.h"

#include <stdio.h>
#include <stdlib.h>

// forward the IP packet from the interface specified by longest_prefix_match, 
// when forwarding the packet, you should check the TTL, update the checksum,
// determine the next hop to forward the packet, then send the packet by 
// iface_send_packet_by_arp
void ip_forward_packet(u32 ip_dst, char *packet, int len)
{
	//fprintf(stderr, "TODO: forward ip packet.\n");
	struct ether_header* eh = (struct ether_header*)(packet);
	struct iphdr* iph = packet_to_ip_hdr(packet);
	struct icmphdr* m_icmph = (struct icmphdr*)(packet + ETHER_HDR_SIZE + IP_HDR_SIZE(iph));
	u32 src = ntohl(iph->saddr);
	rt_entry_t* m_entry = longest_prefix_match(ip_dst);
	rt_entry_t* t_entry = longest_prefix_match(src);

	// fail to find
	if( !m_entry )
	{
		u32 sip = t_entry->iface->ip;
		icmp_send_packet(packet, len, 3, 0, sip);
		free(packet);
		return;
	}

	// when TTL - 1 <= 0
	iph->ttl -= 1;
	if(iph->ttl <= 0)
	{
		u32 sip = t_entry->iface->ip;
		icmp_send_packet(packet, len, 11, 0, sip);
		free(packet);
		return;
	}

	iph->checksum = ip_checksum(iph);
	memcpy(eh->ether_shost, m_entry->iface->mac, ETH_ALEN);
	u32 next_hop = m_entry->gw;
	if(!next_hop) next_hop = ip_dst;

	iface_send_packet_by_arp(m_entry->iface, next_hop, packet, len);
}

// handle ip packet
//
// If the packet is ICMP echo request and the destination IP address is equal to
// the IP address of the iface, send ICMP echo reply; otherwise, forward the
// packet.
void handle_ip_packet(iface_info_t *iface, char *packet, int len)
{
	struct iphdr *ip = packet_to_ip_hdr(packet);
	u32 daddr = ntohl(ip->daddr);
	if (daddr == iface->ip) {
		//fprintf(stderr, "TODO: reply to the sender if it is ping packet.\n");
		u32 dst = ntohl(ip->saddr);
		rt_entry_t* m_entry = longest_prefix_match(dst);
		u32 sip = m_entry->iface->ip;

		icmp_send_packet(packet, len, 0, 0, sip);
		free(packet);
	}
	else {
		ip_forward_packet(daddr, packet, len);
	}
}
