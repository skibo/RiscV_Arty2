/*-
 * Copyright (c) 2016 Thomas Skibo.
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

/*
 * Small driver for Xilinx LogiCORE IP AXI Ethernet Lite Mac.
 * See DS787 from Xilinx for more info.
 */

#include "types.h"
#include "io.h"
#include "ether.h"

#define ETH_TX_PING		(ETH_BASE + 0)		/* tx "ping" buffer */
#define ETH_MDIO_ADDR		(ETH_BASE + 0x07e4)
#define    ETH_MDIO_ADDR_REGA_SHIFT	0
#define    ETH_MDIO_ADDR_REGA_MASK	(0x1f << 0)
#define    ETH_MDIO_ADDR_PHYA_SHIFT	5
#define    ETH_MDIO_ADDR_PHYA_MASK	(0x1f << 5)
#define    ETH_MDIO_ADDR_RNW		(1 << 10)
#define ETH_MDIO_WRITE		(ETH_BASE + 0x07e8)
#define ETH_MDIO_READ		(ETH_BASE + 0x07ec)
#define ETH_MDIO_CTRL		(ETH_BASE + 0x07f0)
#define    ETH_MDIO_CTRL_STATUS		(1 << 0)
#define    ETH_MDIO_CTRL_EN		(1 << 3)
#define ETH_TX_PING_LEN		(ETH_BASE + 0x07f4)
#define ETH_GIE			(ETH_BASE + 0x07f8)
#define    ETH_GIE_BIT			(1 << 31)
#define ETH_TX_PING_CTRL	(ETH_BASE + 0x07fc)
#define    ETH_TX_CTRL_LOOPB		(1 << 4)	/* loop-back enable */
#define    ETH_TX_CTRL_IE		(1 << 3)	/* interrupt enable */
#define    ETH_TX_CTRL_PROG		(1 << 1)	/* program mac addr */
#define    ETH_TX_CTRL_GO		(1 << 0)	/* tx in progress */
#define ETH_TX_PONG		(ETH_BASE + 0x0800)	/* tx "pong" buffer */
#define ETH_TX_PONG_LEN		(ETH_BASE + 0x0ff4)
#define ETH_TX_PONG_CTRL	(ETH_BASE + 0x0ffc)
#define ETH_RX_PING		(ETH_BASE + 0x1000)	/* rx "ping" buffer */

#define ETH_RX_PING_CTRL	(ETH_BASE + 0x17fc)
#define    ETH_RX_CTRL_IE		(1 << 3)	/* interrupt enable */
#define    ETH_RX_CTRL_RDY		(1 << 0)	/* rx data available */
#define ETH_RX_PONG		(ETH_BASE + 0x1800)	/* rx "pong" buffer */
#define ETH_RX_PONG_CTRL	(ETH_BASE + 0x1ffc)

#define RD32(a)		(*(volatile uint32_t *)(a))
#define WR32(a, d)	(*(volatile uint32_t *)(a) = (d))

uint8_t eth_addr[6] = {0x00, 0x00, 0x5e, 0x00, 0xfa, 0xce};
static int tx_pingpong;
static int rx_pingpong;

/* Set the MAC address. */
void
ether_setaddr(const uint8_t *addr)
{
	int i;
	uint32_t *buf = (uint32_t *)ETH_TX_PING;

	/* Wait for buf to be available. */
	while ((RD32(ETH_TX_PING_CTRL) & ETH_TX_CTRL_GO) != 0)
		;

	/* Put 6 byte address in buffer. */
	WR32(&buf[0], addr[0] | ((uint32_t)addr[1] << 8) |
	     ((uint32_t)addr[2] << 16) | ((uint32_t)addr[3] << 24));
	WR32(&buf[1], addr[4] | ((uint32_t)addr[5] << 8));

	for (i = 0; i < 6; i++)
		eth_addr[i] = addr[i];

	/* Go! */
	WR32(ETH_TX_PING_CTRL, ETH_TX_CTRL_GO | ETH_TX_CTRL_PROG);

	/* Wait for program complete. */
	while ((RD32(ETH_TX_PING_CTRL) &
		(ETH_TX_CTRL_GO | ETH_TX_CTRL_PROG)) != 0)
		;
}

/* Transmit a packet. */
void
ether_tx(const uint32_t *data, int len)
{
	int i;
	uint32_t ctrlreg = tx_pingpong ? ETH_TX_PONG_CTRL : ETH_TX_PING_CTRL;
	uint32_t *buf =	(uint32_t *)(tx_pingpong ? ETH_TX_PONG : ETH_TX_PING);

	/* Wait for buf to be available. */
	while ((RD32(ctrlreg) & ETH_TX_CTRL_GO) != 0)
		;

	/* Fill buf. */
	for (i = 0; i < len; i += 4)
		WR32(buf++, *data++);

	/* Set length. */
	WR32(tx_pingpong ? ETH_TX_PONG_LEN : ETH_TX_PING_LEN, len);

	/* Go! */
	WR32(ctrlreg, ETH_TX_CTRL_GO);

	/* Ping pong. */
	tx_pingpong = !tx_pingpong;
}

/* Receive a packet.  Returns maxlen if data availalbe or 0 if none.
 * Hardware doesn't tell us the length of the packet received.
 */
int
ether_rx(uint32_t *data, int maxlen)
{
	int i;
	uint32_t ctrlreg = rx_pingpong ? ETH_RX_PONG_CTRL : ETH_RX_PING_CTRL;
	uint32_t *buf = (uint32_t *)(rx_pingpong ? ETH_RX_PONG : ETH_RX_PING);

	/* Check receive status bit. */
	if ((RD32(ctrlreg) & ETH_RX_CTRL_RDY) == 0) {
		/* Check other buffer status bit in case we are out of sync. */
		ctrlreg = rx_pingpong ? ETH_RX_PING_CTRL : ETH_RX_PONG_CTRL;
		if ((RD32(ctrlreg) & ETH_RX_CTRL_RDY) == 0)
			return 0;
		buf = (uint32_t *)(rx_pingpong ? ETH_RX_PING : ETH_RX_PONG);
		rx_pingpong = !rx_pingpong;
	}

	/* Get data from buffer. */
	for (i = 0; i < maxlen; i += 4)
		*data++ = RD32(buf++);

	/* Clear receive status bit. */
	WR32(ctrlreg, 0);

	/* Ping pong. */
	rx_pingpong = !rx_pingpong;

	return maxlen;
}

/* Write PHY register through MDIO interface. */
int
ether_mdio_wr(uint8_t phy, uint8_t reg, uint16_t data)
{
	/* Wait until not busy. */
	while ((RD32(ETH_MDIO_CTRL) & ETH_MDIO_CTRL_STATUS) != 0)
		;

	/* Write phy address and reg address. */
	WR32(ETH_MDIO_ADDR, ((uint32_t)reg << ETH_MDIO_ADDR_REGA_SHIFT) |
	     ((uint32_t)phy << ETH_MDIO_ADDR_PHYA_SHIFT));

	/* Write data. */
	WR32(ETH_MDIO_WRITE, data);

	/* Go! */
	WR32(ETH_MDIO_CTRL, ETH_MDIO_CTRL_EN | ETH_MDIO_CTRL_STATUS);

	/* Wait until done. */
	while ((RD32(ETH_MDIO_CTRL) & ETH_MDIO_CTRL_STATUS) != 0)
		;

	return 0;
}

/* Read PHY register via MDIO interface. */
int
ether_mdio_rd(uint8_t phy, uint8_t reg)
{
	/* Wait until not busy. */
	while ((RD32(ETH_MDIO_CTRL) & ETH_MDIO_CTRL_STATUS) != 0)
		;

	/* Write phy address and reg address. */
	WR32(ETH_MDIO_ADDR, ((uint32_t)reg << ETH_MDIO_ADDR_REGA_SHIFT) |
	     ((uint32_t)phy << ETH_MDIO_ADDR_PHYA_SHIFT) |
	     ETH_MDIO_ADDR_RNW);

	/* Go! */
	WR32(ETH_MDIO_CTRL, ETH_MDIO_CTRL_EN | ETH_MDIO_CTRL_STATUS);

	/* Wait until done. */
	while ((RD32(ETH_MDIO_CTRL) & ETH_MDIO_CTRL_STATUS) != 0)
		;

	/* Return data. */
	return RD32(ETH_MDIO_READ) & 0xffff;
}
