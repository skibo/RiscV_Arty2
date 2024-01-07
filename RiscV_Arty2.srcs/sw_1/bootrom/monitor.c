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


#include "types.h"
#include "io.h"
#include "console.h"
#include "blinky.h"
#include "sys.h"

static char linebuf[64];
extern int memfault;

static char *
skipspace(char *s) {
	while (*s == ' ' || *s == '\t')
		s++;
	return s;
}

/* Decode a single hex character. */
static uint8_t
gethex1(char c) {
	if (c >= '0' && c <= '9')
		return c-'0';
	else if (c >= 'a' && c <= 'f')
		return c-'a'+10;
	else if (c >= 'A' && c <= 'F')
		return c-'A'+10;
	else
		return 0;
}

/* Decode a hex number up to n characters long. */
static uint32_t
gethex(char **s, int n) {
	uint32_t data = 0;
	while (n > 0 && ((**s >= '0' && **s <= '9') ||
			 (**s >= 'a' && **s <= 'f') ||
			 (**s >= 'A' && **s <= 'F'))) {
		data = (data << 4) | (uint32_t) gethex1(**s);
		(*s)++;
		n--;
	}
	return data;
}

/* Dump memory in bytes. */
static void
dumpbytes(uint32_t addr, int len)
{
	int i;
	uint8_t d8;

	while (len > 0) {
		cons_printf("%8x: ", addr);
		for (i = (addr & 15); i > 0; i--)
			cons_puts("   ");
		do {
			memfault = -1;
			d8 = *(uint8_t *)addr;
			if (memfault > 0) {
				memfault = 0;
				cons_puts("fault!\n");
				return;
			}
			memfault = 0;
			cons_printf("%2x ", d8);
			addr++;
			len--;
		} while (len > 0 && (addr & 15) != 0);
		cons_puts("\n");
		if (cons_pollc() >= 0)
			break;
	}
}

/* Dump memory in 32-bit words. */
static void
dumpmem(uint32_t addr, int len)
{
	int i;
	uint32_t d32;

	addr &= ~3;
	len >>= 2;

	while (len > 0) {
		cons_printf("%8x: ", addr);
		for (i = (addr & 15); i > 0; i -= 4)
			cons_puts("         ");
		do {
			memfault = -1;
			d32 = *(uint32_t *)addr;
			if (memfault > 0) {
				memfault = 0;
				cons_puts("fault!\n");
				return;
			}
			memfault = 0;
			cons_printf("%8x ", d32);
			addr += 4;
			len--;
		} while (len > 0 && (addr & 15) != 0);
		cons_puts("\n");
		if (cons_pollc() >= 0)
			break;
	}
}

static uint32_t startaddr;

/* Handle S records (any line beginning with S).  Set LED to blue if
 * formatting problem or short line.  Set LED to red if checksum is bad.
 * Set LED to green if successful, including ignored commands.
 */
static void
srecord(char *s)
{
	static int srec_err = 0;
	char	type;
	uint8_t	count;
	uint32_t addr;
	uint8_t d8, cksum;

	blink_stop();

	/* Use blank S record to clear error. */
	if (*s == '\0') {
		blink_set_rgb(0, 0);
		srec_err = 0;
		return;
	}

	type = *(s++);
	count = gethex(&s, 2);

	/* Start checksum with count. */
	cksum = count;

	/* Get address. */
	switch (type) {
	case '0':
	case '1':
	case '9':
		/* 16-bit address. */
		addr = gethex(&s, 4);
		count -= 2;
		break;
	case '2':
	case '8':
		/* 24-bit address. */
		addr = gethex(&s, 6);
		count -= 3;
		break;
	case '3':
	case '7':
		/* 32-bit address. */
		addr = gethex(&s, 8);
		count -= 4;
		break;
	}

	/* Checksum address. */
	cksum += (addr & 0xff);
	cksum += ((addr >> 8) & 0xff);
	cksum += ((addr >> 16) & 0xff);
	cksum += ((addr >> 24) & 0xff);

	/* Perform function. */
	switch (type) {
	case '0':
		/* Vendor comment. Ignore. */
		break;
	case '1':
	case '2':
	case '3':
		/* Data line. */
		while (count-- > 0) {
			/* Check for short line. */
			if (s[0] == '\0') {
				srec_err = 1;
				break;
			}

			d8 = gethex(&s, 2);

			if (count > 0) {
				/* Write data. */
				*((volatile uint8_t *)addr) = d8;
				addr++;
				cksum += d8;
			} else {
				/* Check checksum. */
				if (cksum != (d8 ^ 0xff))
					srec_err = 4;
			}
		}
		break;
	case '7':
	case '8':
	case '9':
		/* Start of execution address. */
		startaddr = addr;

		/* Check checksum. */
		d8 = gethex(&s, 2);
		if (cksum != (d8 ^ 0xff))
			srec_err = 4;
		break;
	case '5':
	case '6':
		/* Record count.  ignore. */
		break;
	default:
		srec_err = 1;
	}

	/* Set LED to error code, green if none. */
	blink_set_rgb(0, srec_err == 0 ? 2 : srec_err);
}

void
monitor(void) {
	char *s;
	uint8_t data8;
	uint32_t data32;

	uint32_t arg0, arg1;
	uint8_t cmd;

	cons_puts("\nMonitor:\n");

	linebuf[0] = '\0';

	blink_start();

	for (;;) {
		cons_puts("> ");
		cons_getline(linebuf, sizeof(linebuf));

		s = skipspace(linebuf);
		cmd = *s++;

		switch (cmd) {

		case 'r':
			/* Read word. */
			s = skipspace(s);
			if (!*s)
				break;
			arg0 = gethex(&s, 8);

			memfault = -1;
			data32 = *(uint32_t *)arg0;
			if (memfault > 0) {
				memfault = 0;
				cons_puts("fault!\n");
				break;
			}
			memfault = 0;

			cons_printf("%8x: %8x\n", arg0, data32);
			break;

		case 'w':
			/* Write location. */
			s = skipspace(s);
			if (!*s)
				break;
			arg0 = gethex(&s, 8);
			s = skipspace(s);
			if (!*s)
				break;
			arg1 = gethex(&s, 8);

			cons_printf("%8x <- %8x", arg0, arg1);
			memfault = -1;
			*((uint32_t *)arg0) = arg1;
			if (memfault > 0) {
				memfault = 0;
				cons_puts(" fault!\n");
				break;
			}
			memfault = 0;
			cons_puts("\n");
			break;

		case 'd':
		case 'D':
			/* Dump memory. */
			s = skipspace(s);
			if (*s) {
				arg0 = gethex(&s, 8);
				s = skipspace(s);
			}
			if (*s)
				arg1 = gethex(&s, 8);

			if (cmd == 'D')
				dumpbytes(arg0, arg1);
			else
				dumpmem(arg0, arg1);
			arg0 += arg1;
			break;

		case 'b':
			blink_toggle();
			break;

		case 'B':
			ebreak();
			break;

		case 'S':
			/* Handle S records. */
			srecord(s);
			break;

		case 'g':
			/* Goto location.  Default is startaddr */
			s = skipspace(s);
			if (s[0] != '\0')
				data32 = gethex(&s, 8);
			else
				data32 = startaddr;

			cons_printf("Executing at addr %8x\n", data32);

			(*((void (*)(void))data32))();
			break;

		case '\0':
			break;

		case 'h':
		case '?':
			cons_puts("Commands:\n"
				  "   r <addr> -\t\tread location\n"
				  "   w <addr> <data> -\twrite location\n"
				  "   d <addr> <len> -\tdump memory\n"
				  "   D <addr> <len> -\tdump bytes\n"
				  "   g <addr> -\t\texecute at location\n"
				  "   b -\t\t\ttoggle blinking LEDs.\n"
				  "   B -\t\t\thit breakpoint.\n"
				  "   S -\t\t\tprocess S record.\n"
				);
			break;

		default:
			cons_printf("Unknown command: '%c'.  "
				    "Type h for help\n", cmd);
		}
	}
}
