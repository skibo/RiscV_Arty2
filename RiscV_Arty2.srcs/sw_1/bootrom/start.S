/*-
 * Copyright (c) 2016-2018 Thomas Skibo.
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

	.text
	/* Reset entry.  Called from vect.S */
	.global _start
_start:
	/* Stack at end of memory for now. */
	li      sp, 0x7ff0

	/* Zero out BSS. */
	la      a0, __bss_start
	la      a1, __bss_end
2:
	sw      zero, 0(a0)
	addi    a0, a0, 4
	blt     a0, a1, 2b

	/* Copy initialized data to RAM. */
	la      a0, _data
	la      a1, _edata
	la      a2, _erodata
3:
	lw      a3, 0(a2)
	addi    a2, a2, 4
	sw      a3, 0(a0)
	addi    a0, a0, 4
	blt     a0, a1, 3b

	/* Set MIE bit in mstatus. */
	csrsi   mstatus, 8

	/* Go! */
	jal     main

4:
	/* Hang if we return. */
	j       4b

	.global exc_handle
exc_handle:
	/* save t0. */
	csrw    mscratch, t0

	/* Get cause.  Interrupt? */
	csrr    t0, mcause
	blt     t0, zero, intr_vect

	/* Save all regs except t0 (x5) which is in mscratch. */
	la      t0, xregs

	sw	x0, 0(t0)
	sw	x1, 4(t0)
	sw	x2, 8(t0)
	sw	x3, 12(t0)
	sw	x4, 16(t0)

	sw	x6, 24(t0)
	sw	x7, 28(t0)
	sw	x8, 32(t0)
	sw	x9, 36(t0)
	sw	x10, 40(t0)
	sw	x11, 44(t0)
	sw	x12, 48(t0)
	sw	x13, 52(t0)
	sw	x14, 56(t0)
	sw	x15, 60(t0)
	sw	x16, 64(t0)
	sw	x17, 68(t0)
	sw	x18, 72(t0)
	sw	x19, 76(t0)
	sw	x20, 80(t0)
	sw	x21, 84(t0)
	sw	x22, 88(t0)
	sw	x23, 92(t0)
	sw	x24, 96(t0)
	sw	x25, 100(t0)
	sw	x26, 104(t0)
	sw	x27, 108(t0)
	sw	x28, 112(t0)
	sw	x29, 116(t0)
	sw	x30, 120(t0)
	sw	x31, 124(t0)

	/* Get t0 back from scratch register and save. */
	csrr    t1, mscratch
	sw	t1, 20(t0)

	/* Use exception handler stack. */
	la      sp, xstack + 0x1f0

	csrr    a0, mcause
	csrr    a1, mstatus
	csrr    a2, mepc
	csrr    a3, mbadaddr
	jal     exception

	/* Return value from handler goes back into mstatus. */
	csrw    mstatus, a0

	/* Restore caller saved registers.  Do t0 (x5) last. */
	la      t0, xregs

	lw	x1, 4(t0)
	lw	x2, 8(t0)

	lw	x6, 24(t0)
	lw	x7, 28(t0)
	lw	x10, 40(t0)
	lw	x11, 44(t0)
	lw	x12, 48(t0)
	lw	x13, 52(t0)
	lw	x14, 56(t0)
	lw	x15, 60(t0)
	lw	x16, 64(t0)
	lw	x17, 68(t0)
	lw	x28, 112(t0)
	lw	x29, 116(t0)
	lw	x30, 120(t0)
	lw	x31, 124(t0)

	lw	t0, 20(t0)

	mret

intr_vect:
	la      t0, iregs

	/* Save caller saved regs except t0 (x5) which is in mscratch. */
	sw	x1, 4(t0)
	sw	x2, 8(t0)

	sw	x6, 24(t0)
	sw	x7, 28(t0)
	sw	x10, 40(t0)
	sw	x11, 44(t0)
	sw	x12, 48(t0)
	sw	x13, 52(t0)
	sw	x14, 56(t0)
	sw	x15, 60(t0)
	sw	x16, 64(t0)
	sw	x17, 68(t0)
	sw	x28, 112(t0)
	sw	x29, 116(t0)
	sw	x30, 120(t0)
	sw	x31, 124(t0)

	/* Get t0 back from scratch register and save. */
	csrr    t1, mscratch
	sw	t1, 20(t0)

	/* Use interrupt handler stack. */
	la      sp, istack + 0x1f0

	/* Call interrupt handler:
	 * uint32_t interrupt(mcause, mstatus, mip, mie);
	 */
	csrr    a0, mcause
	csrr    a1, mstatus
	csrr    a2, mip
	csrr    a3, mie
	jal     interrupt

	/* Return value from handler goes back into mstatus. */
	csrw    mstatus, a0

	/* Restore caller saved registers.  Do t0 (x5) last. */
	la      t0, iregs

	lw	x1, 4(t0)
	lw	x2, 8(t0)

	lw	x6, 24(t0)
	lw	x7, 28(t0)
	lw	x10, 40(t0)
	lw	x11, 44(t0)
	lw	x12, 48(t0)
	lw	x13, 52(t0)
	lw	x14, 56(t0)
	lw	x15, 60(t0)
	lw	x16, 64(t0)
	lw	x17, 68(t0)
	lw	x28, 112(t0)
	lw	x29, 116(t0)
	lw	x30, 120(t0)
	lw	x31, 124(t0)

	lw	t0, 20(t0)

	mret

	/* void DELAY(int usecs);
	 * Delay in microseconds.  Assumes 100Mhz clock.
	 */
	.global DELAY
DELAY:
	beq     a0, zero, 3f
1:
	li      t0, 32
2:
	addi    t0, t0, -1
	bnez    t0, 2b
	addi    a0, a0, -1
	bnez    a0, 1b
3:
	ret

	/* void ebreak(void);
	 * Execute an ebreak instruction to dump into debugger.
	 */
	.global ebreak
ebreak:
	ebreak
	ret

	/* void setmepc(uint32_t mepc);
	 * Set mepc csr value.
	 */
	.global setmepc
setmepc:
	csrw	mepc,a0
	ret

	/* void setmtie(void);
	 * Set mtie bit in mie csr (enable interrupts).
	 */
	.global setmtie
setmtie:
	li	t0,128
	csrs	mie,t0
	ret

	/* void clrmtie(void);
	 * Clear mtie bit in mie csr (disable interrupts).
	 */
	.global clrmtie
clrmtie:
	li	t0,128
	csrc	mie,t0
	ret
