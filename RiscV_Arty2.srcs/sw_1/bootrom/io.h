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

#ifndef __IO_H__
#define __IO_H__

#define MTIME_REG	0x00020000
#define MTIMEH_REG	0x00020004
#define MTIMECMP_REG	0x00020008
#define MTIMECMPH_REG	0x0002000c

#define GPIO0_BASE	0x40000000
#define    GPIO0_DATA0		(GPIO0_BASE)
#define    GPIO0_TRI0		(GPIO0_BASE + 4)
#define    GPIO0_DATA1		(GPIO0_BASE + 8)
#define    GPIO0_TRI1		(GPIO0_BASE + 12)
#define GPIO1_BASE	0x40010000
#define    GPIO1_DATA0		(GPIO1_BASE)
#define    GPIO1_TRI0		(GPIO1_BASE + 4)
#define    GPIO1_DATA1		(GPIO1_BASE + 8)
#define    GPIO1_TRI1		(GPIO1_BASE + 12)

#define UART0_BASE	0x40600000
#define UART1_BASE	0x40610000
#define UART_RX_FIFO	0		/* Registers for AXI UART Lite. */
#define UART_TX_FIFO	4		/* See DS741 from Xilinx. */
#define UART_STAT_REG	8
#define    UART_STAT_RX_VALID	(1 << 0)
#define    UART_STAT_RX_FULL	(1 << 1)
#define    UART_STAT_TX_EMPTY	(1 << 2)
#define    UART_STAT_TX_FULL	(1 << 3)
#define    UART_STAT_INTR_EN	(1 << 4)
#define    UART_STAT_OVERRUN	(1 << 5)
#define    UART_STAT_FRAME_ERR	(1 << 6)
#define    UART_STAT_PARITY_ERR	(1 << 7)
#define UART_CTRL_REG	12
#define    UART_CTRL_RST_TX	(1 << 0)
#define    UART_CTRL_RST_RX	(1 << 1)
#define    UART_CTRL_INTR_EN	(1 << 4)

#define ETH_BASE	0x40e00000
#define INTC_BASE	0x41200000

#define RD32(a)		(*(volatile uint32_t *)(a))
#define WR32(a, d)	(*(volatile uint32_t *)(a) = (d))

#endif /* __IO_H__ */
