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
#include "sys.h"
#include "ether.h"

static uint32_t ip_addr = 0xc0a80116; /* 192.168.1.22 */

#define ntohs(x) ((((x) << 8) & 0xff00) | (((x) >> 8) & 0xff))
#define htons(x) ntohs(x)
#define ntohl(x) ((((x) << 24) & 0xff000000) | (((x) << 8) & 0x00ff0000) | \
                  (((x) >> 8) & 0x0000ff00) | (((x) >> 24) & 0x000000ff))
#define htonl(x) ntohl(x)

static void
dumpbytes(uint32_t addr, int size)
{
        int i;
        uint8_t d8;

        while (size > 0) {
                cons_puthex(addr, 8);
                cons_puts(": ");
                for (i = 0; i < 16; i++) {
                        d8 = *(uint8_t *)addr;
                        if (memfault) {
                                memfault = 0;
                                return;
                        }
                        cons_puthex(d8, 2);
                        cons_putchar(' ');
                        addr++;
                        if (--size <= 0)
                                break;
                }
                cons_puts("\n");
                if (cons_pollc() >= 0)
                        break;
        }
}

/* For receive packet data. */
union {
        struct __attribute__((__packed__)) {
                uint8_t daddr[6];
                uint8_t saddr[6];
                uint16_t ethertype;

                union {
                        /* ARP packet */
                        struct {
                                uint16_t hwtype;
                                uint16_t proto;
                                uint8_t hwlen;
                                uint8_t prlen;
                                uint16_t opcode;
                                uint8_t srcmac[6];
                                uint8_t srcip[4];
                                uint8_t dstmac[6];
                                uint8_t dstip[4];
                        } arp;
                        /* IP packet */
                        struct {
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
                                uint8_t data[];
                        } ip;
                };
        };
        uint32_t data[512];
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

void
input_arp(void)
{
        int i;
        uint32_t reqip;

        /* Check hardware and proto address type and length. */
        if (ntohs(pkt.arp.hwtype) != 1 || ntohs(pkt.arp.proto) != 0x800 ||
            pkt.arp.hwlen != 6 || pkt.arp.prlen != 4) {
                cons_puts("Unhandled ARP hwtype or proto\r\n");
                return;
        }

        /* Only answer requests. */
        if (ntohs(pkt.arp.opcode) != 1)
                return;

        /* Looking for us? */
        for (i = 0; i < 4; i++)
                reqip = (reqip << 8) | pkt.arp.dstip[i];
        if (reqip != ip_addr)
                return;

        /* Turn around req as response. */
        for (i = 0; i < 6; i++) {
                pkt.daddr[i] = pkt.arp.srcmac[i];
                pkt.saddr[i] = eth_addr[i];
                pkt.arp.dstmac[i] = pkt.arp.srcmac[i];
                pkt.arp.srcmac[i] = eth_addr[i];
        }
        for (i = 0; i < 4; i++) {
                pkt.arp.dstip[i] = pkt.arp.srcip[i];
                pkt.arp.srcip[i] = ((ip_addr >> ((3 - i) * 8)) & 0xff);
        }
        pkt.arp.opcode = htons(2); /* reply */

        cons_puts("Replying to ARP request.\r\n");

        ether_tx(pkt.data, sizeof(pkt.arp) + 14);
}

void
input_ip(void)
{
        int i;
        int hlen;
        struct icmp_s {
                uint8_t type;
                uint8_t code;
                uint16_t csum;
                uint16_t id;
                uint16_t seq;
                uint8_t data[];
        } *icmp;
        struct udp_s {
                uint16_t sport;
                uint16_t dport;
                uint16_t len;
                uint16_t csum;
                uint8_t data[];
        } *udp;

        /* Sanity check header length. */
        hlen = 4 * (pkt.ip.vers_hlen & 0xf);
        if (hlen < 20) {
                cons_puts("Short IP header!\r\n");
                return;
        }

        /* Check header checksum. */
        if (ipsum((uint16_t *)&pkt.ip, hlen / 2) != 0xffff) {
                cons_puts("IP header checksum bad!\r\n");
                return;
        }

        /* Be sure addressed to me. */
        if ((pkt.ip.dstaddr[0] != ((ip_addr >> 24) & 0xff) ||
             pkt.ip.dstaddr[1] != ((ip_addr >> 16) & 0xff) ||
             pkt.ip.dstaddr[2] != ((ip_addr >> 8) & 0xff) ||
             pkt.ip.dstaddr[3] != (ip_addr & 0xff)) &&
            (pkt.ip.dstaddr[0] != 0xff || pkt.ip.dstaddr[1] != 0xff ||
             pkt.ip.dstaddr[2] != 0xff || pkt.ip.dstaddr[3] != 0xff)) {
                cons_puts("IP Packet not addressed to me:\r\n");
                return;
        }

        /* No fragments! */
        if ((ntohs(pkt.ip.frag) & 0x3fff) != 0) {
                cons_puts("Dropping fragmented IP packet.\r\n");
                return;
        }

        /* Protocol? */
        if (pkt.ip.proto == 0x01) {
                /* ICMP */
                int icmp_len = ntohs(pkt.ip.totlen) - hlen;
                icmp = (struct icmp_s *)((char *)&pkt.ip + hlen);

                /* Check ICMP checksum. */
                if (ipsum((uint16_t *)icmp, icmp_len / 2) != 0xffff) {
                        cons_puts("ICMP checksum bad!\r\n");
                        return;
                }

                if (icmp->type == 8 /* Echo Request */) {
                        cons_puts("ICMP Echo Request.\r\n");

                        /* Turn around packet as reply. */
                        for (i = 0; i < 6; i++) {
                                pkt.daddr[i] = pkt.saddr[i];
                                pkt.saddr[i] = eth_addr[i];
                        }
                        for (i = 0; i < 4; i++) {
                                pkt.ip.dstaddr[i] = pkt.ip.srcaddr[i];
                                pkt.ip.srcaddr[i] =
                                        (ip_addr >> ((3 - i) * 8)) & 0xff;
                        }
                        pkt.ip.ttl = 64;

                        /* Recalculate IP header csum. */
                        pkt.ip.hdrcsum = 0;
                        pkt.ip.hdrcsum = htons(ipsum((uint16_t *)&pkt.ip,
                                                     hlen / 2) ^ 0xffff);

                        icmp->type = 0 /* Echo Reply */;

                        /* Recalculate ICMP checksum. */
                        icmp->csum = 0;
                        icmp->csum = htons(ipsum((uint16_t *)icmp,
                                                 icmp_len / 2) ^ 0xffff);

                        /* Transmit. */
                        ether_tx(pkt.data, hlen + icmp_len + 24);
                }
        } else if (pkt.ip.proto == 0x11) {
                /* UDP */
                udp = (struct udp_s *)pkt.ip.data;

                /* Check UDP checksum. */
                if (udp->csum != 0) {
                        uint32_t csum;
                        struct {
                                uint8_t srcip[4];
                                uint8_t dstip[4];
                                uint8_t zero;
                                uint8_t proto;
                                uint16_t udplen;
                        } pseudo;

                        for (i = 0; i < 4; i++) {
                                pseudo.srcip[i] = pkt.ip.srcaddr[i];
                                pseudo.dstip[i] = pkt.ip.dstaddr[i];
                        }
                        pseudo.zero = 0;
                        pseudo.proto = 0x11;
                        pseudo.udplen = udp->len;
                        csum = ipsum((uint16_t *)&pseudo, sizeof(pseudo) / 2);
                        csum += ipsum((uint16_t *)udp, ntohs(udp->len) / 2);
                        csum = (csum >> 16) + (csum & 0xffff);

                        if (csum != 0xffff) {
                                cons_puts("UDP checksum bad!\r\n");
                                return;
                        }
                }

                cons_puts("UDP Packet: src port = ");
                cons_puthex(ntohs(udp->sport), 4);
                cons_puts(" dst port = ");
                cons_puthex(ntohs(udp->dport), 4);
                cons_puts(" len (not incl hdr) = ");
                cons_puthex(ntohs(udp->len) - 8, 4);
                cons_puts("\r\n");
        } else {
                cons_puts("Unsupported IP proto: ");
                cons_puthex(pkt.ip.proto, 2);
                cons_puts("\r\n");
        }
}

void
do_pingtest(void)
{
        int i;

        cons_puts("Ping test...\n\r");

        for (;;) {
                /* Get packet. */
                while (ether_rx(pkt.data, sizeof(pkt.data)) == 0) {
                        if (cons_pollc() >= 0)
                                return;
                }

                /* Output DST and SRC addresses. */
                cons_puts("Dst: ");
                for (i= 0; i < 6; i++) {
                        cons_puthex(pkt.daddr[i], 2);
                        cons_putchar(' ');
                }
                cons_puts("\n\rSrc: ");
                for (i= 0; i < 6; i++) {
                        cons_puthex(pkt.saddr[i], 2);
                        cons_putchar(' ');
                }
                cons_puts("\n\r");

                /* Ethertype. */
                switch (ntohs(pkt.ethertype)) {
                case 0x800:
                        cons_puts("IP Packet: \n\r");
                        if ((pkt.ip.vers_hlen & 0xf0) != 0x40) {
                                cons_puts("Bad vers_len\n\r");
                                dumpbytes((uint32_t)&pkt, sizeof(pkt));
                        }
                        else
                                input_ip();
                        break;
                case 0x806:
                        /* ARP. */
                        input_arp();
                        break;
                default:
                        cons_puts("\n\rUnhandled Ethertype: ");
                        cons_puthex(ntohs(pkt.ethertype), 4);
                        cons_puts("\n\r");
                        dumpbytes((uint32_t)&pkt, sizeof(pkt.data));
                }
        }
}
