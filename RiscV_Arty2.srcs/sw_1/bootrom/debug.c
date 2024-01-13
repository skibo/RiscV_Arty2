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
#include "sys.h"
#include "io.h"

#ifdef DEBUG_SERIAL
#include "console.h"
#endif

#define SIGINT		2
#define SIGILL		4
#define SIGTRAP		5
#define SIGSEGV         11

static char dbgpkt[384];
static uint32_t xregs2[32];

static void
dbg_putchar(char c)
{
	while ((RD32(UART1_BASE + UART_STAT_REG) & UART_STAT_TX_FULL) != 0)
		;
	WR32(UART1_BASE + UART_TX_FIFO, c);

#ifdef DEBUG_SERIAL
	cons_putchar(c);
#endif
}

/* Returns -1 if there is no read data. */
static int
dbg_pollc(void)
{
	if ((RD32(UART1_BASE + UART_STAT_REG) & UART_STAT_RX_VALID) != 0)
		return (RD32(UART1_BASE + UART_RX_FIFO));
	else
		return (-1);
}

/* Waits for read data. */
static int
dbg_getchar(void)
{
	int c;

	while ((c = dbg_pollc()) < 0)
		;

#ifdef DEBUG_SERIAL
	cons_putchar(c);
#endif

	return (c);
}

/* Single character hex to integer. */
static int
dbg_hextoi(char c)
{
	if (c >= '0' && c <= '9')
		return (c - '0');
	else if (c >= 'a' && c <= 'f')
		return (c - 'a' + 10);
	else if (c >= 'A' && c <= 'F')
		return (c - 'A' + 10);
	else
		return (-1);
}

/* Get a hex value of up to n characters from a string pointer and advance
 * the string pointer to the end of the hex value.  The end of the hex
 * value is either n characters or the first non-hex character.
 */
static unsigned int
dbg_gethex(const char **ps, int n)
{
	unsigned int x  = 0;
	int nyb;

	while (n-- > 0) {
		if (**ps == '\0')
			break;
		nyb = dbg_hextoi(*(*ps)++);
		if (nyb < 0)
			break;
		x = (x << 4) | nyb;
	}

	return (x);
}

/* Get a 32-bit hex value in local byte order. */
static uint32_t
dbg_gethex32(const char **ps)
{
	uint32_t x;

	x = dbg_gethex(ps, 2);
	x |= dbg_gethex(ps, 2) << 8;
	x |= dbg_gethex(ps, 2) << 16;
	x |= dbg_gethex(ps, 2) << 24;

	return (x);
}

/* Receive a serial protocol packet. */
static void
dbg_getpkt(void)
{
	int c;
	int i;
	uint8_t cksum;
	for (;;) {
		while (dbg_getchar() != '$')
			;
		i = 0;
		cksum = 0;
		while (i < sizeof(dbgpkt)) {
			c = dbg_getchar();
			if (c == '#')
				break;
			if (c == 0x7d) {
				c = dbg_getchar();
				c ^= 0x20;
			}
			dbgpkt[i++] = c;
			cksum += c;
		}

		/* Overflow? */
		if (c != '#')
			continue;

		/* Check checksum. */
		c = dbg_getchar();
		if (dbg_hextoi(c) != (cksum >> 4)) {
#ifdef DEBUG_SERIAL
			cons_puts("dbg_getpkt: bad cksum1\n");
#endif
			dbg_putchar('-');
			continue;
		}
		c = dbg_getchar();
		if (dbg_hextoi(c) != (cksum & 0xf)) {
#ifdef DEBUG_SERIAL
			cons_puts("dbg_getpkt: bad cksum2\n");
#endif
			dbg_putchar('-');
			continue;
		}

		dbgpkt[i] = '\0';
		dbg_putchar('+');
		break;
	}
}

/* Single character integer to hex character. */
static char
dbg_itohex(int x)
{
	if (x > 9)
		return ('A' + x - 10);
	else
		return ('0' + x);
}

/* Put an integer into a string as a hex using n characters. */
static void
dbg_puthex(char *s, int x, int n)
{
	while (n-- > 0)
		*s++ = dbg_itohex((x >> (n * 4)) & 0xf);
}

/* Transmit a serial debug packet. */
static void
dbg_putpkt(char *pkt)
{
	int i;
	int c;
	int tries = 6;
	uint8_t cksum;

	do {
		dbg_putchar('$');
		i = 0;
		cksum = 0;
		while (pkt[i] != '\0') {
			dbg_putchar(pkt[i]);
			cksum += pkt[i];
			i++;
		}
		dbg_putchar('#');
		dbg_putchar(dbg_itohex(cksum >> 4));
		dbg_putchar(dbg_itohex(cksum & 0xf));

		for (i = 0; i < 5000000; i++)
			if ((c = dbg_pollc()) >= 0)
				break;

#ifdef DEBUG_SERIAL
		cons_putchar(c);
#endif
	} while (c != '+' && --tries > 0);
}

static int
dbg_strcmp(const char *s1, const char *s2)
{
	while (*s1 == *s2 && *s1 != '\0')
		s1++,s2++;
	return (*s1 != '\0' || *s2 != '\0');
}

/* Handle query commands. */
static void
dbg_query(void)
{
	if (dbg_strcmp(dbgpkt, "qOffsets") == 0)
		dbg_putpkt("Text=0;Data=0;Bss=0");
	else
		dbg_putpkt("");
}

/* Put a 32-bit value into a string in local byte order. */
static void
dbg_puthex32(char *s, uint32_t x)
{
	dbg_puthex(s, x & 0xff, 2);
	dbg_puthex(s + 2, (x >> 8) & 0xff, 2);
	dbg_puthex(s + 4, (x >> 16) & 0xff, 2);
	dbg_puthex(s + 6, (x >> 24) & 0xff, 2);
}

/* Handle get register requests. */
static void
dbg_getregs(uint32_t xregs[], uint32_t mepc)
{
	int i;

	/* x0-x31 general registers. */
	for (i = 0; i < 32; i++)
		dbg_puthex32(dbgpkt + i * 8, xregs[i]);

	/* PC */
	dbg_puthex32(dbgpkt + 32 * 8, mepc);

	dbgpkt[33 * 8] = '\0';
	dbg_putpkt(dbgpkt);
}

static void
dbg_setregs(uint32_t xregs[], uint32_t *pmepc, const char *s)
{
	int i;

	/* x0-x31 general registers. */
	for (i = 0; i < 32; i++)
		xregs[i] = dbg_gethex32(&s);

	/* PC */
	*pmepc = dbg_gethex32(&s);

	/* XXX: if (*s != '#') error!; */
	dbg_putpkt("OK");
}

static void
dbg_getmem(const char *s)
{
	uint32_t addr;
	int len;
	uint32_t d32;
	uint8_t d8;
	int i;

	addr = dbg_gethex(&s, 17);
	len = dbg_gethex(&s, 17);

	i = 0;
	while (len > 0) {

		/* Do reads using 32-bit accesses if possible. */
		if ((addr & 3) == 0 && len > 3) {
			memfault = -1;
			d32 = *(uint32_t *)addr;
			if (memfault > 0) {
				memfault = 0;
				dbg_putpkt("E0E"); /* EFAULT */
				return;
			}
			memfault = 0;
			addr += 4;
			len -= 4;
			dbg_puthex32(dbgpkt + i, d32);
			i += 8;
		} else {
			memfault = -1;
			d8 = *(uint8_t *)addr;
			if (memfault > 0) {
				memfault = 0;
				dbg_putpkt("E0E"); /* EFAULT */
				return;
			}
			memfault = 0;
			addr++;
			len--;
			dbg_puthex(dbgpkt + i, d8, 2);
			i += 2;
		}

		/* Are we running out of room in dbgpkt? */
		if (i >= sizeof(dbgpkt) - 8) {
			dbg_putpkt("E16"); /* EINVAL */
			return;
		}
	}

	dbgpkt[i] = '\0';
	dbg_putpkt(dbgpkt);
}


static void
dbg_setmem(const char *s)
{
	uint32_t addr;
	int len;
	uint32_t d32;
	uint8_t d8;
	int i;

	addr = dbg_gethex(&s, 17);
	len = dbg_gethex(&s, 17);

	i = 0;
	while (len > 0) {
		/* Do writes using 32-bit accesses if possible. */
		if ((addr & 3) == 0 && len > 3) {
			d32 = dbg_gethex32(&s);
			memfault = -1;
			*(uint32_t *)addr = d32;
			if (memfault > 0) {
				memfault = 0;
				dbg_putpkt("E0E"); /* EFAULT */
				return;
			}
			memfault = 0;
			addr += 4;
			len -= 4;
		} else {
			d8 = dbg_gethex(&s, 2);
			memfault = -1;
			*(uint8_t *)addr = d8;
			if (memfault > 0) {
				memfault = 0;
				dbg_putpkt("E0E"); /* EFAULT */
				return;
			}
			memfault = 0;
			addr++;
			len--;
		}
	}
	dbg_putpkt("OK");
}

static void
dbg_cont_step(const char *s, uint32_t *pmepc)
{
	char cmd;
	uint32_t sig;

	cmd = *s++;
	if (cmd == 'C' || cmd == 'S')
		sig = dbg_gethex(&s, 17);
	if (*s != '\0')
		*pmepc = dbg_gethex(&s, 17);
}

static int signal;

static void
dbg_putsig(void)
{
	char buf[4];

	buf[0] = 'S';
	dbg_puthex(&buf[1], signal, 2);
	buf[3] = '\0';
	dbg_putpkt(buf);
}


void
dbg_exception(uint32_t mcause, uint32_t mstatus, uint32_t mepc,
	      uint32_t mbadaddr, uint32_t xregs[])
{
	int i;

	/* Save exception registers in case we nest exceptions due to
	 * memory faults.
	 */
	for (i = 0; i < 32; i++)
		xregs2[i] = xregs[i];

	switch (mcause) {
	case 0:  /* Instruction address misaligned */
	case 1:  /* Instruction access fault */
	case 4:  /* Load address misaligned */
	case 5:  /* Load access fault */
	case 6:  /* Store / AMO address misaligned */
	case 7:  /* Store / AMO access fault */
	case 12: /* Instruction page fault */
	case 13: /* Load page fault */
	case 15: /* Store / AMO page fault */
		signal = SIGSEGV;
		break;
	case 2:  /* Illegal instruction */
		signal = SIGILL;
		break;
	case 3: /* Breakpoint */
		if (mepc == (uint32_t)ebreak)
			mepc += 4;
		/* FALLTHROUGH */
	default:
		signal = SIGTRAP;
		break;
	}

	/* Respond with SIGINT signal. */
	dbg_putsig();

	/* Respond to commands. */
	for (;;) {
		dbg_getpkt();

		switch(dbgpkt[0]) {
		case'g':
			dbg_getregs(xregs2, mepc);
			break;
		case 'G':
			dbg_setregs(xregs2, &mepc, dbgpkt + 1);
			break;
		case 'm':
			dbg_getmem(dbgpkt + 1);
			break;
		case 'M':
			dbg_setmem(dbgpkt + 1);
			break;
		case '?':
			dbg_putsig();
			break;
		case 'q':
			dbg_query();
			break;
		case 'C':
		case 'S':
		case 'c':
		case 's':
			dbg_cont_step(dbgpkt, &mepc);
			for (i = 0; i < 32; i++)
				xregs[i] = xregs2[i];
			setmepc(mepc);
			return;
		default:
			/* Empty response. */
			dbg_putpkt("");
		}
	}
}
