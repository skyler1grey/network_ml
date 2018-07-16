/*
 * decode.hpp
 *
 *  Created on: Jul 11, 2018
 *      Author: root
 */
#ifndef DECODE_HPP_
#define DECODE_HPP_

#define MAX_CAPLEN 1500
#define ETH_LEN 0
#define IP_CHECK 1

#include <pcap.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/if_ether.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>
#include <vector>
#include <algorithm>
#include <iostream>

typedef struct __attribute__ ((__packed__)) FlowKey {
    // 8 (4*2) bytes
    uint32_t src_ip; // source IP address
    uint32_t dst_ip;
    // 4 (2*2) bytes
    uint16_t src_port;
    uint16_t dst_port;
    //1 bytes
    uint8_t proto;
} flow_key_t;

typedef struct Tuple {
    /******************
    ** keys
    ******************/
    flow_key_t key;
    // 1 byte
    // only used in multi-thread environment
    uint8_t flag;

    /******************
    ** values
    ******************/
    // 8 bytes
    double pkt_ts; // timestamp of the packet
    // 8 bytes
    uint64_t seq;
    // 8 bytes
    int64_t size; //inner IP datagram length(header + data)
} tuple_t;

typedef struct {
	pcap_t* pcap;
}adapter_t;

enum PACKET_STATUS {
    STATUS_INIT = 0,  //initial state
    STATUS_VALID, //valid packets
    //error status
    STATUS_NON_IP, //not an IP packet
    STATUS_IP_NOT_FULL, //IP not full packets
    STATUS_IP_VER_FAIL, //IP version failed
    STATUS_IP_CHKSUM_FAIL, //IP checksum failed
    STATUS_IP_FRAG, // IP fragments
    STATUS_TCP_NOT_FULL, // TCP header not full
    STATUS_UDP_NOT_FULL, // UDP header not full
    STATUS_ICMP_NOT_FULL, // ICMP header not full
    STATUS_UNDEFINED, // (not used for now)
    STATUS_NON_GTP, // not a valid GTP packet
    STATUS_NON_GPRS // not a valid GPRS packet
};


adapter_t * pcap_init(char * file)
{
	adapter_t* ret =(adapter_t*) calloc(1, sizeof(adapter_t));

	char errbuf[PCAP_ERRBUF_SIZE];
	if ((ret->pcap = pcap_open_offline(file, errbuf)) == NULL)
	{
		printf("%s\n",errbuf);
		printf("cant open pcap file \n");
		return ret;
	}
	return ret;
}

void adapter_destroy(adapter_t* adapter) {
	pcap_close(adapter->pcap);
};

inline static unsigned short in_chksum_ip(unsigned short* w, int len)
{
    long sum = 0;

    while (len > 1) {
        sum += *w++;
        if (sum & 0x80000000) /* if high order bit sen, fold */
            sum = (sum & 0xFFFF) + (sum >> 16);
        len -= 2;
    }

    if (len)
        sum += *w;

    while (sum >> 16)
        sum = (sum & 0xFFFF) + (sum >> 16);

    return ~sum;
};

enum PACKET_STATUS decode(const uint8_t* pkt,
        uint32_t cap_len,
        uint32_t act_len,
        double ts,
        tuple_t* p) {
    struct ether_header* eth_hdr;
    struct ip* ip_hdr;
    struct tcphdr* tcp_hdr;
    struct udphdr* udp_hdr;
    int eth_len = ETH_LEN;
    enum PACKET_STATUS status;

    status = STATUS_VALID;


    //error checking(Ethernet level)
    if (eth_len == 14) {
        eth_hdr = (struct ether_header*)pkt;
        if (ntohs(eth_hdr->ether_type) == ETHERTYPE_VLAN) {
            eth_len = 18;
        }
        else if (ntohs(eth_hdr->ether_type) != ETH_P_IP) {
            status = STATUS_NON_IP;
        }
    }
    else if (eth_len == 4) {
        if (ntohs(*(uint16_t*)(pkt + 2)) != ETH_P_IP) {
            status = STATUS_NON_IP;
        }
    }
    else if (eth_len != 0) {
        // unkown ethernet header length
        status = STATUS_NON_IP;
    }

    uint32_t len = cap_len - eth_len;

    //error checking(IP level)
    ip_hdr = (struct ip*)(pkt + eth_len);
    // i) IP header length check
    // LOG_MSG("check 1\n");
    if ((int)len < (ip_hdr->ip_hl << 2)) {
        status = STATUS_IP_NOT_FULL;
    }
    // ii) IP version check
    // LOG_MSG("check 2\n");
    if (ip_hdr->ip_v != 4) {
        status = STATUS_IP_VER_FAIL;
    }
    // iii) IP checksum check
    // LOG_MSG("check 3\n");
    if (IP_CHECK && in_chksum_ip((unsigned short*)ip_hdr, ip_hdr->ip_hl << 2)) {
        status = STATUS_IP_CHKSUM_FAIL;
    }

    // LOG_MSG("check 4\n");
    // error checking (TCP\UDP\ICMP layer test)
    if (ip_hdr->ip_p == IPPROTO_TCP) {
        //see if the TCP header is fully captured
        tcp_hdr = (struct tcphdr*)((uint8_t*)ip_hdr + (ip_hdr->ip_hl << 2));
        if ((int)len < (ip_hdr->ip_hl << 2) + (tcp_hdr->doff <<2)) {
            status = STATUS_TCP_NOT_FULL;
        }
    }
    else if (ip_hdr->ip_p == IPPROTO_UDP) {
        //see if the UDP header is fully captured
        if ((int)len < (ip_hdr->ip_hl << 2) + 8) {
            status = STATUS_UDP_NOT_FULL;
        }
    }
    else if (ip_hdr->ip_p == IPPROTO_ICMP) {
        //see if the ICMP header is fully captured
        if ((int)len < (ip_hdr->ip_hl << 2) + 8) {
            status = STATUS_ICMP_NOT_FULL;
        }
    }

    switch (status) {
        default:
            break;
    }
    if (status != STATUS_VALID)
        return status;

    //assign the fields
    p->key.src_ip = ip_hdr->ip_src.s_addr;
    p->key.dst_ip = ip_hdr->ip_dst.s_addr;
    p->key.proto = ip_hdr->ip_p;
    p->pkt_ts = ts;
    p->size = ntohs(ip_hdr->ip_len);

    if (ip_hdr->ip_p == IPPROTO_TCP) {
        //TCP
        tcp_hdr = (struct tcphdr*)((uint8_t*)ip_hdr + (ip_hdr->ip_hl << 2));
        p->key.src_port = ntohs(tcp_hdr->source);
        p->key.dst_port = ntohs(tcp_hdr->dest);
    }
    else if (ip_hdr->ip_p == IPPROTO_UDP) {
        //UDP
        udp_hdr = (struct udphdr*)((uint8_t*)ip_hdr + (ip_hdr->ip_hl << 2));
        p->key.src_port = ntohs(udp_hdr->source);
        p->key.dst_port = ntohs(udp_hdr->dest);
    } else {
        // Other L4
        p->key.src_port = 0;
        p->key.dst_port = 0;
    }
    return status;
};

int adapter_next(adapter_t* adapter, tuple_t* p, enum PACKET_STATUS* status) {
	double pkt_ts; //packet timestamp
	int pkt_len; //packet snap length
	const u_char* pkt; //raw packet
	uint8_t pkt_data[MAX_CAPLEN];
	struct pcap_pkthdr hdr;

	pkt = pcap_next(adapter->pcap, &hdr);
	if (pkt == NULL) {
		return -1;
	}
	pkt_ts = (double)hdr.ts.tv_usec / 1000000 + hdr.ts.tv_sec;
	pkt_len = hdr.caplen < MAX_CAPLEN ? hdr.caplen : MAX_CAPLEN;
	memcpy(pkt_data, pkt, pkt_len);

	*status = decode(pkt_data, pkt_len, hdr.len, pkt_ts, p);
	return 0;
}


template <typename T>
double fangcha(std::vector<T> resultSet)
{
	double sum = std::accumulate(resultSet.begin(),resultSet.end(),0.0);
	//std::cout<<sum<<std::endl;
	double mean =  sum / resultSet.size()*1.0; //均值


	//std::cout<<mean<<std::endl;
	double accum  = 0.0;

	std::for_each (std::begin(resultSet), std::end(resultSet), [&](const double d) {

		accum  += (d-mean)*(d-mean);

	});

	//std::cout<<accum<<std::endl;


	double stdev = sqrt(accum/(resultSet.size()-1));
	return stdev;

}




#endif /* DECODE_HPP_ */
