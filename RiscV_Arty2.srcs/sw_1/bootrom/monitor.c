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

char linebuf[64];
int memfault;

static char *
skipspace(char *s) {
        while (*s == ' ' || *s == '\t')
                s++;
        return s;
}

static void
dumpmem(uint32_t addr, int count)
{
        int i;
        uint32_t data;

        while (count > 0) {
                cons_puthex(addr, 8);
                cons_puts(": ");
                for (i = 0; i < 4; i++) {
                        data = *(uint32_t *)addr;
                        if (memfault) {
                                memfault = 0;
                                return;
                        }
                        cons_puthex(data, 8);
                        cons_putchar(' ');
                        addr += 4;
                        if (--count <= 0)
                                break;
                }
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
                        arg0 = cons_gethex(&s, 8);

                        data32 = *(uint32_t *)arg0;

                        if (memfault) {
                                memfault = 0;
                                break;
                        }

                        cons_puthex(arg0, 8);
                        cons_puts(": ");
                        cons_puthex(data32, 8);
                        cons_puts("\r\n");
                        break;

                case 'w':
                        /* Write location. */
                        s = skipspace(s);
                        arg0 = cons_gethex(&s, 8);
                        s = skipspace(s);
                        arg1 = cons_gethex(&s, 8);

                        cons_puthex(arg0, 8);
                        cons_puts(" <- ");
                        cons_puthex(arg1, 8);
                        *((uint32_t *)arg0) = arg1;
                        cons_puts("\r\n");
                        if (memfault)
                                memfault = 0;
                        break;

                case 'd':
                        /* Dump memory. */
                        s = skipspace(s);
                        arg0 = cons_gethex(&s, 8);
                        s = skipspace(s);
                        arg1 = cons_gethex(&s, 8);

                        dumpmem(arg0, arg1);
                        break;
                        
                case 'q':
                        return;

                case 'h':
                case '?':
                        cons_puts("Commands:\r\n"
                                  "   r <addr> -\t\tread location\r\n"
                                  "   w <addr> <data> -\twrite location\r\n"
                                  "   d <addr> <hexnum> -\tdump memory\r\n"
                                  "   q -\t\t\tquit monitor.\n"
                                );
                        break;

                default:
                        cons_puts("Uknown command: ");
                        cons_putchar(cmd);
                        cons_puts("  type h for help.\r\n");
                }
        }
}

