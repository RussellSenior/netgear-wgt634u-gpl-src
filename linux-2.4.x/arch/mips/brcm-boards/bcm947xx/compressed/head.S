/*
 * BCM947XX Self-Booting Linux
 *
 * Code should be position-independent until it copies itself to SDRAM.
 *
 * Copyright 2001-2003, Broadcom Corporation
 * All Rights Reserved.
 * 
 * THIS SOFTWARE IS OFFERED "AS IS", AND BROADCOM GRANTS NO WARRANTIES OF ANY
 * KIND, EXPRESS OR IMPLIED, BY STATUTE, COMMUNICATION OR OTHERWISE. BROADCOM
 * SPECIFICALLY DISCLAIMS ANY IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A SPECIFIC PURPOSE OR NONINFRINGEMENT CONCERNING THIS SOFTWARE.
 *
 * $Id$
 */

#include <asm/asm.h>
#include <asm/regdef.h>
#include <asm/addrspace.h>	
#include <asm/mipsregs.h>
#include <bcm4710.h>

	.text
	LEAF(startup)
	.set	noreorder

/* Dont look at nvram now for dram initalization. Hope cfe/bootloader did it. Fix it latter  */                                                               
                                                                               
	blt     t0, t1, inram                                                   
	nop                                                                     
                                                                               
#if 0
	/* Check if we booted from SDRAM */
	bal	1f
	nop
1:	li	t0, 0x1fffffff
	and	t0, t0, ra
	li	t1, BCM4710_FLASH
	blt     t0, t1, inram
	nop

	/* Initialize SDRAM */
	li	t0, KSEG1ADDR(BCM4710_FLASH)
	la	t1, text_start
	la	t2, board_draminit
	sub	t2, t2, t1
	add	t2, t2, t0
	jalr	t2
	nop

	/* Copy self to SDRAM */
	li	a0, BCM4710_FLASH
	la	a1, text_start
	la	a2, input_data
1:	lw	t0, 0(a0)
	sw	t0, 0(a1)
	add	a0, 4
	add	a1, 4
	blt	a1, a2, 1b
	nop
#endif
inram:
	/* Set up stack pointer */
	li      sp, 0x80800000 - 4

	/* Clear BSS */	
	la	a0, bss_start
	la	a1, bss_end
1:	sw	zero, 0(a0)
	addi	a0, a0, 4
	blt	a0, a1, 1b
	nop

	/* Jump to C */
	la	t0, c_main
	jal	t0
	move	a0, ra

	/* Embedded NVRAM */
	.balign	0x400	
	.space  0x2000
	
	.set reorder
	END(startup)
