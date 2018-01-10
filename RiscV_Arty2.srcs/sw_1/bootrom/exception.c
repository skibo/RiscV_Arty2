/*-
 * Copyright (c) 2016,2018 Thomas Skibo.
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
#include "io.h"

int memfault;

uint32_t xstack[64];
uint32_t xregs[32];

static void
dumpregs(uint32_t *r)
{
        int i;

        for (i = 0; i < 32; i++) {
                cons_puthex(r[i], 8);
                if ((i & 3) == 3)
                        cons_puts("\n");
                else
                        cons_putchar(' ');
        }
}

void
exception(uint32_t mcause, uint32_t mstatus, uint32_t mepc, uint32_t mbadaddr)
{
        cons_puts("\nException!\nmcause=");
        cons_puthex(mcause, 8);
        cons_puts(" mstatus=");
        cons_puthex(mstatus, 8);
        cons_puts(" mepc=");
        cons_puthex(mepc, 8);
        cons_puts(" mbadaddr=");
        cons_puthex(mbadaddr, 8);
        cons_puts("\n\n");
    
        dumpregs(xregs);

        if (mcause == 5)
                memfault++;

        /* Restart after offending instruction. */
        mepc += 4;
        asm("csrw mepc, %0" : : "r"(mepc));
}

uint32_t istack[64];
uint32_t iregs[32];

void
interrupt(void)
{
        uint32_t a;

        cons_puts("\nInterrupt!? mepc=");
        asm ("csrr %0, mepc" : "=r" (a));
        cons_puthex(a, 8);
        cons_puts("\n");
}
