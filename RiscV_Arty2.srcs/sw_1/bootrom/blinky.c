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
#include "sys.h"
#include "blinky.h"

#define BLINK_INTERVAL	10000001U	/* 100ms / 10ns */

static int blink_enable;
static int blink_ctr;

void
timer_intr(void)
{
	int color = ((blink_ctr >> 4) & 7);
	uint32_t tmrh, tmr;

	/* Do something interesting with LEDs. */
	WR32(GPIO0_DATA1,
	     ((blink_ctr & 1) ? color : 0) |
	     ((blink_ctr & 2) ? (color << 3) : 0) |
	     ((blink_ctr & 4) ? (color << 6) : 0) |
	     ((blink_ctr & 8) ? (color << 9) : 0));

	WR32(GPIO0_DATA0, blink_ctr & 0xf);

	blink_ctr++;

	/* Set up next interrupt. */
	do {
		tmrh = RD32(MTIMEH_REG);
		tmr = RD32(MTIME_REG);
	} while (tmrh != RD32(MTIMEH_REG));

	if (tmr + BLINK_INTERVAL < tmr)
		tmrh++;
	tmr += BLINK_INTERVAL;

	WR32(MTIMECMPH_REG, tmrh);
	WR32(MTIMECMP_REG, tmr);
}

void
blink_start(void)
{
	blink_enable = 1;
	setmtie();	/* Enable interrupts. */
	timer_intr();
}

void
blink_stop(void)
{
	clrmtie();	/* Disable interrupts. */
	blink_enable = 0;

	/* Turn off LEDs. */
	WR32(GPIO0_DATA1, 0);
	WR32(GPIO0_DATA0, 0);

	blink_ctr = 0;
}

void
blink_toggle(void)
{
	if (blink_enable)
		blink_stop();
	else
		blink_start();
}

void
blink_set_rgb(int i, int rgb)
{
	uint32_t d1 = RD32(GPIO0_DATA1);

	d1 &= ~(7 << (i * 3));
	d1 |= (rgb << (i * 3));
	WR32(GPIO0_DATA1, d1);
}
