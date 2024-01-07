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
#include "sys.h"
#include "console.h"
#include "io.h"

int memfault;

uint32_t xstack[128];
uint32_t xregs[32];

uint32_t istack[128];
uint32_t iregs[32];
uint32_t savedmstatus; /* mstatus saved during interrupt. */

extern void timer_intr(void);
#ifdef GDBSTUB
extern void dbg_exception(uint32_t, uint32_t, uint32_t, uint32_t, uint32_t []);
#endif

static void
dumpregs(uint32_t *r)
{
	int i;
	static char *rnames[] = {
		"x0/z  ", "x1/ra ", "x2/sp ", "x3/gp ",
		"x4/tp ", "x5/t0 ", "x6/t1 ", "x7/t2 ",
		"x8/s0 ", "x9/s1 ", "x10/a0", "x11/a1",
		"x12/a2", "x13/a3", "x14/a4", "x15/a5",
		"x16/a6", "x17/a7", "x18/s2", "x19/s3",
		"x20/s4", "x21/s5", "x22/s6", "x23/s7",
		"x24/s8", "x25/s9", "x26/sA", "x27/sB",
		"x28/t3", "x29/t4", "x30/t5", "x31/t6"
	};

	for (i = 0; i < 32; i++) {
		cons_printf("%s=%8x", rnames[i], r[i]);
		if ((i & 3) == 3)
			cons_puts("\n");
		else
			cons_putchar(' ');
	}
}

uint32_t
exception(uint32_t mcause, uint32_t mstatus, uint32_t mepc, uint32_t mbadaddr)
{
	/* Catch memory faults from monitor. */
	if (memfault < 0 && mcause >= 4 && mcause <= 7) {
		memfault = mcause;

		/* Restart after offending instruction. */
		mepc += 4;
		asm volatile ("csrw mepc, %0" : : "r"(mepc));

		return mstatus;
	}

	cons_printf("\nException!\n");
	cons_printf("   mcause=%8x mstatus=%8x mepc=%8x mbadaddr=%8x\n",
		    mcause, mstatus, mepc, mbadaddr);

	dumpregs(xregs);
#ifdef GDBSTUB
	dbg_exception(mcause, mstatus, mepc, mbadaddr, xregs);
#else
	/* If exception in ebreak(), allow to continue. */
	if (mepc == (uint32_t)ebreak) {
		mepc += 4;
		asm volatile ("csrw mepc, %0" : : "r"(mepc));
		return mstatus;
	}

	for (;;)
		;
#endif
	return mstatus;
}

uint32_t
interrupt(uint32_t mcause, uint32_t mstatus, uint32_t mip, uint32_t mie)
{

	if ((mip & mie & (1 << 7)) != 0) {
		/* Timer interrupt. */
		timer_intr();
	}

	return mstatus;
}
