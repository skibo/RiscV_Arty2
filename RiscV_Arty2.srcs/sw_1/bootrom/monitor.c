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
#include "sys.h"
#include "ether.h"

char linebuf[64];
int memfault;

extern void do_pingtest(void);

static char *
skipspace(char *s) {
        while (*s == ' ' || *s == '\t')
                s++;
        return s;
}

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

void
dumpmem(uint32_t addr, int size)
{
        int i;
        uint8_t d8;

        while (size > 0) {
                cons_printf("%8x: ", addr);
                if ((addr & 15) != 0)
                        for (i = (addr & 15); i < 16; i++)
                                cons_puts("   ");
                do {
                        d8 = *(uint8_t *)addr;
                        if (memfault) {
                                memfault = 0;
                                cons_puts("fault!\n");
                                return;
                        }
                        cons_printf("%2x ", d8);
                        addr++;
                        size--;
                } while (size > 0 && (addr & 15) != 0);
                cons_puts("\n");
                if (cons_pollc() >= 0)
                        break;
        }
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
        memfault = 0;

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

                        data32 = *(uint32_t *)arg0;

                        if (memfault) {
                                memfault = 0;
                                break;
                        }

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
                        *((uint32_t *)arg0) = arg1;
                        if (memfault) {
                                cons_puts(" fault! ");
                                memfault = 0;
                        }
                        cons_puts("\n");
                        break;

                case 'd':
                        /* Dump memory. */
                        s = skipspace(s);
                        if (!*s)
                                break;
                        arg0 = gethex(&s, 8);
                        s = skipspace(s);
                        if (!*s)
                                break;
                        arg1 = gethex(&s, 8);

                        dumpmem(arg0, arg1);
                        break;

                case 'M':
                        /* MDIO read. */
                        s = skipspace(s);
                        if (!*s)
                                break;
                        arg0 = gethex(&s, 8);

                        data32 = ether_mdio_rd(1, arg0);

                        cons_printf("MDIO %2x: %4x\n", arg0, data32);
                        break;

                case 'N':
                        /* MDIO write. */
                        s = skipspace(s);
                        if (!*s)
                                break;
                        arg0 = gethex(&s, 8);
                        s = skipspace(s);
                        if (!*s)
                                break;
                        arg1 = gethex(&s, 8);

                        cons_printf("MDIO %2x <- %4x\n", arg0, arg1);
                        ether_mdio_wr(1, arg0, arg1);
                        break;

                case 'p':
                        do_pingtest();
                        break;

                case 'q':
                        return;

                case 'h':
                case '?':
                        cons_puts("Commands:\n"
                                  "   r <addr> -\t\tread location\n"
                                  "   w <addr> <data> -\twrite location\n"
                                  "   d <addr> <num> -\tdump memory\n"
                                  "   M <addr> -\t\tMDIO read\n"
                                  "   N <addr> <data> -\tMDIO write\n"
                                  "   p -\t\t\tping test.\n"
                                  "   q -\t\t\tquit monitor.\n"
                                );
                        break;

                default:
                        cons_printf("Uknown command: %c.  "
                                    "Type h for help\n", cmd);
                }
        }
}
