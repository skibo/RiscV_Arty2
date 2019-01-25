/*-
 * Copyright (c) 2018 Thomas Skibo.
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
 * main.c
 */

#include "types.h"
#include "io.h"
#include "console.h"
#include "sys.h"

/* This is here so that we'll have at least one piece of initialized data. */
static char *say_hi = "\n\nHello, Arty!\n\nHit m to drop into monitor.\n";

void
main(void)
{
        int c, i, j = 0;

	/* Enable LED tri-state outputs. */
        WR32(GPIO0_TRI0, 0);
        WR32(GPIO0_TRI1, 0);

        cons_puts(say_hi);

        cons_puts("Date compiled: ");
        cons_puts(__DATE__);
        cons_puts("\nTime compiled: ");
        cons_puts(__TIME__);
        cons_puts("\n\nReady!\n");

        /* Blink the LEDs in an interesting way. */
        for (;;) {
                for (i = 0; i < 64; i++) {
                        int color = ((j >> 4) & 7);
                        WR32(GPIO0_DATA1,
                             ((j & 1) ? color : 0) |
                             ((j & 2) ? (color << 3) : 0) |
                             ((j & 4) ? (color << 6) : 0) |
                             ((j & 8) ? (color << 9) : 0));
                        WR32(GPIO0_DATA0, j & 0xf);
                        j++;

                        /* delay 100ms */
                        DELAY(100000);

                        // poll keyboard
                        if ((c = cons_pollc()) == 'm')
                                monitor();
                        else if (c >= 0)
                                break;
                }
                cons_printf("Buttons = %x  Switches = %x\n",
                            RD32(GPIO1_DATA0),
                            RD32(GPIO1_DATA1));
        }
}

