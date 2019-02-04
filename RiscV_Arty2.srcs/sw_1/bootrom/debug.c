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
#include "io.h"

static void
dbg_putchar(char c)
{
        while ((RD32(UART1_BASE + UART_STAT_REG) & UART_STAT_TX_FULL) != 0)
                ;
        WR32(UART1_BASE + UART_TX_FIFO, c);
}

static void
dbg_puts(const char *s)
{
        while (*s) {
                if (*s == '\n')
                        dbg_putchar('\r');
                dbg_putchar(*s++);
        }
}

/* Returns -1 if there is no read data because that is what
 * the UART_DATA_REG does.
 */
static int
dbg_pollc(void)
{
        if ((RD32(UART1_BASE + UART_STAT_REG) & UART_STAT_RX_VALID) != 0)
                return RD32(UART1_BASE + UART_RX_FIFO);
        else
                return -1;
}

static int
dbg_getchar(void)
{
        int c;

        while ((c = dbg_pollc()) < 0)
                ;

        return c;
}

void
dbg_init(void)
{
        dbg_puts("Hello, UART1 and debugger!\nPress any key...");
        (void)dbg_getchar();
        dbg_puts("\n");
}
