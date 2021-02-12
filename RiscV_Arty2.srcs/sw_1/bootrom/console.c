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
#include <stdarg.h>

#include "types.h"
#include "io.h"
#include "console.h"

void
cons_putchar(char c)
{
	while ((RD32(UART0_BASE + UART_STAT_REG) & UART_STAT_TX_FULL) != 0)
		;
	WR32(UART0_BASE + UART_TX_FIFO, c);
}

void
cons_puts(const char *s)
{
	while (*s) {
		if (*s == '\n')
			cons_putchar('\r');
		cons_putchar(*s++);
	}
}

/* Return input character if available.  Return -1 if there is no read data. */
int
cons_pollc(void)
{
	if ((RD32(UART0_BASE + UART_STAT_REG) & UART_STAT_RX_VALID) != 0)
		return RD32(UART0_BASE + UART_RX_FIFO);
	else
		return -1;
}

/* Wait for input character and return it. */
int
cons_getchar(void)
{
	int c;

	while ((c = cons_pollc()) < 0)
		;

	return c;
}

/* Crude getline with single line recall. */
void
cons_getline(char *s, int maxlen)
{
	int c;
	int i = 0;

	for (;;) {
		c = cons_getchar() & 0x7f;

		if (c == '\r') {
			/* Return. */
			cons_puts("\n");
			s[i] = '\0';
			return;
		} else if (c == 0x7f || c == '\b') {
			/* Backspace. */
			if (i > 0) {
				cons_puts("\b \b");
				i--;
			}
		} else if (c == '\020') {
			/* Ctrl-P */
			while (i < maxlen - 1 && s[i])
				cons_putchar(s[i++]);
		} else if ((c & 0x60) != 0 && i < maxlen - 1) {
			/* All other printable characters. */
			cons_putchar(c);
			s[i++] = c;
		}
	}
}

static void
cons_puthex(unsigned int x, int n)
{
	int i, nyb;

	for (i = 7; i >= 0; i--) {
		nyb = (x >> (i * 4)) & 0xf;
		if (nyb == 0 && (n > i || i == 0))
			cons_putchar('0');
		else if (nyb != 0) {
			if (nyb > 9)
				cons_putchar('A' - 10 + nyb);
			else
				cons_putchar('0' + nyb);
			n = 99;
		}
	}
}

static void
cons_putdec(int x)
{
	int d = 1000000000;
	int e, lead0 = 0;

	if (x == 0) {
		cons_putchar('0');
		return;
	} else if (x < 0) {
		cons_putchar('-');
		x = -x;
	}

	while (d > 0) {
		e = x / d;
		if (e == 0 && lead0)
			cons_putchar('0');
		else if (e > 0) {
			cons_putchar('0' + e);
			lead0++;
			x -= e * d;
		}
		d /= 10;
	}
}

/* Crude implementation of printf(). */
int
cons_printf(const char *fmt, ...)
{
	const char *s = fmt;
	va_list valist;
	int w;

	va_start(valist, fmt);

	while (*s) {
		if (*s == '%') {
			s++;

			w = 0;
			if (*s >= '0' && *s <= '9')
				w = *s++ - '0';

			if (*s == '\0')
				break;

			switch (*s) {
			case 'x':
			case 'X':
				cons_puthex(va_arg(valist, uint32_t), w);
				break;
			case 'd':
				cons_putdec(va_arg(valist, int));
				break;
			case 'c':
				cons_putchar(va_arg(valist, int));
				break;
			case 's':
				cons_puts(va_arg(valist, const char *));
				break;
			defualt:
				cons_putchar(*s);
				break;
			}
		} else if (*s == '\n') {
			cons_putchar('\r');
			cons_putchar(*s);
		} else
			cons_putchar(*s);
		s++;
	}

	va_end(valist);
}
