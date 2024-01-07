/*-
 * Copyright (c) 2019 Thomas Skibo.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include "types.h"
#include "console.h"
#include "ether.h"

static uint32_t ip_addr = 0xc0a80116; /* 192.168.1.22 */

#define ntohs(x) ((((x) << 8) & 0xff00) | (((x) >> 8) & 0xff))
#define htons(x) ntohs(x)
#define ntohl(x) ((((x) << 24) & 0xff000000) | (((x) << 8) & 0x00ff0000) | \
		  (((x) >> 8) & 0x0000ff00) | (((x) >> 24) & 0x000000ff))
#define htonl(x) ntohl(x)

/* For receive and trasnmit packet data. */
static struct {
	uint8_t daddr[6];
	uint8_t saddr[6];
	uint16_t ethertype;
	union {
		/* ARP packet */
		struct arp_s {
			uint16_t hwtype;
			uint16_t proto;
			uint8_t hwlen;
			uint8_t prlen;
			uint16_t opcode;
			uint8_t srcmac[6];
			uint8_t srcip[4];
			uint8_t dstmac[6];
			uint8_t dstip[4];
		} arppkt;

		/* IP packet */
		struct ip_s {
			uint8_t vers_hlen;
			uint8_t svc_type;
			uint16_t totlen;
			uint16_t id;
			uint16_t frag;
			uint8_t ttl;
			uint8_t proto;
			uint16_t hdrcsum;
			uint8_t srcaddr[4];
			uint8_t dstaddr[4];
			union {
				struct icmp_s {
					uint8_t type;
					uint8_t code;
					uint16_t csum;
					uint16_t id;
					uint16_t seq;
					uint8_t data[];
				} icmp;
				struct udp_s {
					uint16_t sport;
					uint16_t dport;
					uint16_t len;
					uint16_t csum;
					uint8_t data[];
				} udp;
				uint8_t data[1480];
			};
		} ippkt;

		uint8_t data[1500];
	} u;
} pkt;

static uint16_t
ipsum(uint16_t *data, int words)
{
	int i;
	uint32_t csum = 0;

	for (i = 0; i < words; i++)
		csum += ntohs(data[i]);

	return (uint16_t) (csum >> 16) + (csum & 0xffff);
}

/* Handle incoming ARP packet. */
static void
input_arp(void)
{
	int i;
	uint32_t reqip;

	/* Check hardware and proto address type and length. */
	if (ntohs(pkt.u.arppkt.hwtype) != 1 ||
	    ntohs(pkt.u.arppkt.proto) != 0x800 ||
	    pkt.u.arppkt.hwlen != 6 || pkt.u.arppkt.prlen != 4) {
		cons_puts("Unhandled ARP hwtype or proto\n");
		return;
	}

	/* Only answer requests. */
	if (ntohs(pkt.u.arppkt.opcode) != 1)
		return;

	/* Looking for us? */
	for (i = 0; i < 4; i++)
		reqip = (reqip << 8) | pkt.u.arppkt.dstip[i];
	cons_printf("ARP Request for %d.%d.%d.%d\n",
		    pkt.u.arppkt.dstip[0], pkt.u.arppkt.dstip[1],
		    pkt.u.arppkt.dstip[2], pkt.u.arppkt.dstip[3]);
	if (reqip != ip_addr)
		return;

	/* Turn around req as response. */
	for (i = 0; i < 6; i++) {
		pkt.daddr[i] = pkt.u.arppkt.srcmac[i];
		pkt.saddr[i] = eth_addr[i];
		pkt.u.arppkt.dstmac[i] = pkt.u.arppkt.srcmac[i];
		pkt.u.arppkt.srcmac[i] = eth_addr[i];
	}
	for (i = 0; i < 4; i++) {
		pkt.u.arppkt.dstip[i] = pkt.u.arppkt.srcip[i];
		pkt.u.arppkt.srcip[i] = ((ip_addr >> ((3 - i) * 8)) & 0xff);
	}
	pkt.u.arppkt.opcode = htons(2); /* reply */

	cons_puts("Replying to ARP request.\n");

	ether_tx((uint32_t *)&pkt, sizeof(pkt.u.arppkt) + 14);
}

/* Handle incoming IP packet. */
static void
input_ip(void)
{
	int i;
	int hlen;

	/* Sanity check header length. */
	hlen = 4 * (pkt.u.ippkt.vers_hlen & 0xf);
	if (hlen < 20) {
		cons_puts("Short IP header!\n");
		return;
	}

	/* Check header checksum. */
	if (ipsum((uint16_t *)&pkt.u.ippkt, hlen / 2) != 0xffff) {
		cons_puts("IP header checksum bad!\n");
		return;
	}

	/* Be sure packet is addressed to me. */
	if ((pkt.u.ippkt.dstaddr[0] != ((ip_addr >> 24) & 0xff) ||
	     pkt.u.ippkt.dstaddr[1] != ((ip_addr >> 16) & 0xff) ||
	     pkt.u.ippkt.dstaddr[2] != ((ip_addr >> 8) & 0xff) ||
	     pkt.u.ippkt.dstaddr[3] != (ip_addr & 0xff)) &&
	    (pkt.u.ippkt.dstaddr[0] != 0xff ||
	     pkt.u.ippkt.dstaddr[1] != 0xff ||
	     pkt.u.ippkt.dstaddr[2] != 0xff ||
	     pkt.u.ippkt.dstaddr[3] != 0xff)) {
		cons_puts("IP Packet not addressed to me: ");
		cons_printf("%d.%d.%d.%d\n", pkt.u.ippkt.dstaddr[0],
			    pkt.u.ippkt.dstaddr[1], pkt.u.ippkt.dstaddr[2],
			    pkt.u.ippkt.dstaddr[3]);
		return;
	}

	/* No fragments! */
	if ((ntohs(pkt.u.ippkt.frag) & 0x3fff) != 0) {
		cons_puts("Dropping fragmented IP packet.\n");
		return;
	}

	/* Protocol? */
	switch (pkt.u.ippkt.proto) {
	case 0x01: /* ICMP */
		int icmp_len = ntohs(pkt.u.ippkt.totlen) - hlen;

		/* Check ICMP checksum. */
		if (ipsum((uint16_t *)&pkt.u.ippkt.icmp, icmp_len / 2) !=
		    0xffff) {
			cons_puts("ICMP checksum bad!\n");
			return;
		}

		if (pkt.u.ippkt.icmp.type == 8 /* Echo Request */) {
			cons_puts("ICMP Echo Request: from ");
			cons_printf("%d.%d.%d.%d\n", pkt.u.ippkt.srcaddr[0],
				    pkt.u.ippkt.srcaddr[1],
				    pkt.u.ippkt.srcaddr[2],
				    pkt.u.ippkt.srcaddr[3]);

			/* Turn around packet as reply. */
			for (i = 0; i < 6; i++) {
				pkt.daddr[i] = pkt.saddr[i];
				pkt.saddr[i] = eth_addr[i];
			}
			for (i = 0; i < 4; i++) {
				pkt.u.ippkt.dstaddr[i] =
					pkt.u.ippkt.srcaddr[i];
				pkt.u.ippkt.srcaddr[i] =
					(ip_addr >> ((3 - i) * 8)) & 0xff;
			}
			pkt.u.ippkt.ttl = 64;

			/* Recalculate IP header csum. */
			pkt.u.ippkt.hdrcsum = 0;
			pkt.u.ippkt.hdrcsum =
				htons(ipsum((uint16_t *)&pkt.u.ippkt,
					    hlen / 2) ^ 0xffff);

			pkt.u.ippkt.icmp.type = 0 /* Echo Reply */;

			/* Recalculate ICMP checksum. */
			pkt.u.ippkt.icmp.csum = 0;
			pkt.u.ippkt.icmp.csum = htons(ipsum((uint16_t *)
						       &pkt.u.ippkt.icmp,
						       icmp_len / 2) ^ 0xffff);

			/* Transmit. */
			ether_tx((uint32_t *)&pkt, hlen + icmp_len + 24);
		}
		break;

	case 0x11: /* UDP */

		/* Check UDP checksum. */
		if (pkt.u.ippkt.udp.csum != 0) {
			uint32_t csum;
			struct {
				uint8_t srcip[4];
				uint8_t dstip[4];
				uint8_t zero;
				uint8_t proto;
				uint16_t udplen;
			} pseudo;

			for (i = 0; i < 4; i++) {
				pseudo.srcip[i] = pkt.u.ippkt.srcaddr[i];
				pseudo.dstip[i] = pkt.u.ippkt.dstaddr[i];
			}
			pseudo.zero = 0;
			pseudo.proto = 0x11;
			pseudo.udplen = pkt.u.ippkt.udp.len;
			csum = ipsum((uint16_t *)&pseudo, sizeof(pseudo) / 2);
			csum += ipsum((uint16_t *)&pkt.u.ippkt.udp,
				      ntohs(pkt.u.ippkt.udp.len) / 2);
			csum = (csum >> 16) + (csum & 0xffff);

			if (csum != 0xffff) {
				cons_puts("UDP checksum bad!\n");
				return;
			}
		}

		cons_printf("UDP Packet: "
			    "src port = %d dst port = %d  len = %d\n",
			    ntohs(pkt.u.ippkt.udp.sport),
			    ntohs(pkt.u.ippkt.udp.dport),
			    ntohs(pkt.u.ippkt.udp.len));
		break;
	default:
		cons_printf("Unsupported IP proto: 0x%2x\n",
			    pkt.u.ippkt.proto);

	} // switch (pkt.u.ippkt.proto)
}

void
main(void)
{
	int i;

	cons_puts("Ping test...my address is 192.168.1.22\n");
	cons_puts("Hit any key to exit.\n");

	for (;;) {
		/* Get packet. */
		while (ether_rx((uint32_t *)&pkt, sizeof(pkt)) == 0) {
			if (cons_pollc() >= 0)
				return;
		}

		/* Output DST and SRC addresses. */
		cons_puts("Dst: ");
		for (i= 0; i < 6; i++)
			cons_printf("%2x ", pkt.daddr[i]);
		cons_puts("\nSrc: ");
		for (i= 0; i < 6; i++)
			cons_printf("%2x ", pkt.saddr[i]);
		cons_puts("\n");

		/* Ethertype. */
		switch (ntohs(pkt.ethertype)) {
		case 0x800:
			cons_puts("IP Packet: \n");
			if ((pkt.u.ippkt.vers_hlen & 0xf0) != 0x40) {
				cons_printf("Bad vers_len.\n");
				break;
			}
			input_ip();
			break;
		case 0x806:
			/* ARP. */
			cons_puts("ARP Packet:\n");
			input_arp();
			break;
		default:
			cons_printf("\n\rUnhandled Ethertype: %4x\n",
				    ntohs(pkt.ethertype));
		}
	}
}
