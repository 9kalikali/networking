#include "arpcache.h"
#include "arp.h"
#include "ether.h"
#include "packet.h"
#include "icmp.h"
#include "ip.h"
#include "rtable.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>

static arpcache_t arpcache;

// initialize IP->mac mapping, request list, lock and sweeping thread
void arpcache_init()
{
	bzero(&arpcache, sizeof(arpcache_t));

	init_list_head(&(arpcache.req_list));

	pthread_mutex_init(&arpcache.lock, NULL);

	pthread_create(&arpcache.thread, NULL, arpcache_sweep, NULL);
}

// release all the resources when exiting
void arpcache_destroy()
{
	pthread_mutex_lock(&arpcache.lock);

	struct arp_req *req_entry = NULL, *req_q;
	list_for_each_entry_safe(req_entry, req_q, &(arpcache.req_list), list) {
		struct cached_pkt *pkt_entry = NULL, *pkt_q;
		list_for_each_entry_safe(pkt_entry, pkt_q, &(req_entry->cached_packets), list) {
			list_delete_entry(&(pkt_entry->list));
			free(pkt_entry->packet);
			free(pkt_entry);
		}

		list_delete_entry(&(req_entry->list));
		free(req_entry);
	}

	pthread_kill(arpcache.thread, SIGTERM);

	pthread_mutex_unlock(&arpcache.lock);
}

// lookup the IP->mac mapping
//
// traverse the table to find whether there is an entry with the same IP
// and mac address with the given arguments
int arpcache_lookup(u32 ip4, u8 mac[ETH_ALEN])
{
	//fprintf(stderr, "TODO: lookup ip address in arp cache.\n");
	pthread_mutex_lock(&arpcache.lock);
	//int i;
	for(int i = 0; i < MAX_ARP_SIZE; i++)
	{
		if(arpcache.entries[i].ip4 == ip4 && arpcache.entries[i].valid == 1)
		{
			for(int j = 0; j < ETH_ALEN; j++)
				mac[j] = arpcache.entries[i].mac[j];
			pthread_mutex_unlock(&arpcache.lock);
			return 1;
		}
	}
	pthread_mutex_unlock(&arpcache.lock);
	return 0;
}

// append the packet to arpcache
//
// Lookup in the list which stores pending packets, if there is already an
// entry with the same IP address and iface (which means the corresponding arp
// request has been sent out), just append this packet at the tail of that entry
// (the entry may contain more than one packet); otherwise, malloc a new entry
// with the given IP address and iface, append the packet, and send arp request.
void arpcache_append_packet(iface_info_t *iface, u32 ip4, char *packet, int len)
{
	//fprintf(stderr, "TODO: append the ip address if lookup failed, and send arp request if necessary.\n");
	pthread_mutex_lock(&arpcache.lock);

	struct arp_req* req_entry = NULL;
	struct arp_req* req_q;
	list_for_each_entry_safe(req_entry, req_q, &(arpcache.req_list), list)
	{
		if(req_entry->iface->ip == iface->ip && 
			req_entry->ip4 == ip4)
		{
			//printf("this is %d th q.\n", q++);
			struct cached_pkt* pkt_entry;
			pkt_entry = (struct cached_pkt*)malloc(sizeof(struct cached_pkt));
			pkt_entry->packet = packet;
			pkt_entry->len = len;
			list_add_tail( &(pkt_entry->list), 
							&(req_entry->cached_packets));
			pthread_mutex_unlock(&arpcache.lock);
			return;
		}
	}

	struct arp_req* new_req;
	new_req = (struct arp_req*)malloc(sizeof(struct arp_req));
	new_req->iface = iface;
	new_req->ip4 = ip4;
	new_req->sent = time(NULL);
	new_req->retries = 0;
	init_list_head( &(new_req->cached_packets) );
	list_add_tail( &(new_req->list), 
					&(arpcache.req_list));

	struct cached_pkt* new_pkt_entry;
	new_pkt_entry = (struct cached_pkt*)malloc(sizeof(struct cached_pkt));
	new_pkt_entry->packet = packet;
	new_pkt_entry->len = len;
	list_add_tail( &(new_pkt_entry->list),
					&(new_req->cached_packets) );
	pthread_mutex_unlock(&arpcache.lock);
	arp_send_request(iface, ip4);

}

// insert the IP->mac mapping into arpcache, if there are pending packets
// waiting for this mapping, fill the ethernet header for each of them, and send
// them out
void arpcache_insert(u32 ip4, u8 mac[ETH_ALEN])
{
	//fprintf(stderr, "TODO: insert ip->mac entry, and send all the pending packets.\n");
	int flag = 0;
	for(int i = 0; i < MAX_ARP_SIZE; i++)
	{
		if(arpcache.entries[i].ip4 == 0)
		{
			pthread_mutex_lock(&arpcache.lock);
			arpcache.entries[i].ip4 = ip4;
			memcpy(arpcache.entries[i].mac, mac, ETH_ALEN);
			arpcache.entries[i].added = time(NULL);
			arpcache.entries[i].valid = 1;
			pthread_mutex_unlock(&arpcache.lock);
			flag = 1;
			break;
		}
	}

	if(flag == 0)
	{
		// if not found, randomly delete
		int idx = rand()%MAX_ARP_SIZE;
		pthread_mutex_lock(&arpcache.lock);
		arpcache.entries[idx].ip4 = ip4;
		memcpy(arpcache.entries[idx].mac, mac, ETH_ALEN);
		arpcache.entries[idx].added = time(NULL);
		arpcache.entries[idx].valid = 1;
		pthread_mutex_unlock(&arpcache.lock);
	}

	struct arp_req* req_entry = NULL;
	struct arp_req* req_q;

	list_for_each_entry_safe(req_entry, req_q, &(arpcache.req_list), list)
	{
		if(req_entry->ip4 == ip4)
		{
			struct cached_pkt *pkt_entry = NULL, *pkt_q;
			list_for_each_entry_safe(pkt_entry, pkt_q, &(req_entry->cached_packets), list)
			{
				char *packet = pkt_entry->packet;
				int len = pkt_entry->len;
				struct ether_header *eh = (struct ether_header *)malloc(sizeof(*packet)); //(struct ehter_header *)&(*packet);
				memcpy(&eh, &packet, sizeof(*packet));
				memcpy(eh->ether_dhost, mac, ETH_ALEN);
				iface_send_packet(req_entry->iface, packet, len);
				list_delete_entry( &(pkt_entry->list) );
			}
		}
		list_delete_entry( &(req_entry->list) );
	}
}

// sweep arpcache periodically
//
// For the IP->mac entry, if the entry has been in the table for more than 15
// seconds, remove it from the table.
// For the pending packets, if the arp request is sent out 1 second ago, while 
// the reply has not been received, retransmit the arp request. If the arp
// request has been sent 5 times without receiving arp reply, for each
// pending packet, send icmp packet (DEST_HOST_UNREACHABLE), and drop these
// packets.
void *arpcache_sweep(void *arg) 
{
	while (1) {
		sleep(1);
		//fprintf(stderr, "TODO: sweep arpcache periodically: remove old entries, resend arp requests .\n");
		pthread_mutex_unlock(&arpcache.lock);
		pthread_mutex_lock(&arpcache.lock);

		for(int i = 0; i < MAX_ARP_SIZE; i++)
		{
			if(time(NULL) - arpcache.entries[i].added >= 15 &&
				arpcache.entries[i].ip4 != 0)
			{
				arpcache.entries[i].ip4 = 0;
				arpcache.entries[i].valid = 0;
			}
		}

		struct arp_req *pos, *q;
		time_t now = time((time_t*)NULL);

		list_for_each_entry_safe(pos, q, &(arpcache.req_list), list)
		{
			if(now - pos->sent > 1 && pos->retries < 3)
			{
				iface_info_t *iface = pos->iface;
				u32 ip4 = pos->ip4;
				arp_send_request(iface, ip4);
				pos->sent = now;
				pos->retries += 1;
			}
			else if(pos->retries > 2)
			{
				struct cached_pkt *req_ip_pkt, *n;
				list_for_each_entry_safe(req_ip_pkt, n, &(pos->cached_packets), list)
				{
					char* packet = (char *)req_ip_pkt->packet;
					struct iphdr* m_iph = packet_to_ip_hdr(packet);
					// u32 src_ip = ntohl(m_iph->saddr);
					// u32 dst_ip = ntohl(m_iph->daddr);
					pthread_mutex_unlock(&arpcache.lock);
					u32 dst = ntohl(m_iph->saddr);
					rt_entry_t* m_entry = longest_prefix_match(dst);
					u32 sip = m_entry->iface->ip;

					icmp_send_packet( (char*)req_ip_pkt->packet, req_ip_pkt->len, 3, 1, sip);
					pthread_mutex_lock(&arpcache.lock);
					list_delete_entry( &(pos->list) );
				}
			}
		}
		pthread_mutex_unlock(&arpcache.lock);
	}

	return NULL;
}
