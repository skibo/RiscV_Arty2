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

void
cons_putchar(char c)
{
        while ((RD32(UART_STAT_REG) & UART_STAT_TX_FULL) != 0)
                ;
        WR32(UART_TX_FIFO, c);
}

void
cons_puthex(unsigned int x, int n)
{
        int y;

        while (--n >= 0) {
                y = (x >> (n << 2)) & 15;
                if (y > 9)
                        cons_putchar('A' - 10 + y);
                else
                        cons_putchar('0' + y);
        }
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

/* Returns -1 if there is no read data because that is what
 * the UART_DATA_REG does.
 */
int
cons_pollc(void)
{
        if ((RD32(UART_STAT_REG) & UART_STAT_RX_VALID) != 0)
                return RD32(UART_RX_FIFO);
        else
                return -1;
}

int
cons_getchar(void)
{
        int c;

        while ((c = cons_pollc()) < 0)
                ;

        return c;
}

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

static uint8_t
cons_gethex1(char c) {
        if (c >= '0' && c <= '9')
                return c-'0';
        else if (c >= 'a' && c <= 'f')
                return c-'a'+10;
        else if (c >= 'A' && c <= 'F')
                return c-'A'+10;
        else
                return 0;
}

uint32_t
cons_gethex(char **s, int n) {
        uint32_t data = 0;
        while (n > 0 && ((**s >= '0' && **s <= '9') ||
                         (**s >= 'a' && **s <= 'f') ||
                         (**s >= 'A' && **s <= 'F'))) {
                data = (data << 4) | (uint32_t) cons_gethex1(**s);
                (*s)++;
                n--;
        }
        return data;
}
