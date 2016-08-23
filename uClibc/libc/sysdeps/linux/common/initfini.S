	.file	1 "initfini.c"
	.abicalls
 #APP
	
/*@HEADER_ENDS*/
 #NO_APP
	.text
	.align	2
	.globl	dummy
	.ent	dummy
dummy:
	.frame	$sp,32,$31		# vars= 0, regs= 2/0, args= 16, extra= 8
	.mask	0x90000000,-4
	.fmask	0x00000000,0
	.set	noreorder
	.cpload	$25
	.set	reorder
	subu	$sp,$sp,32
	.cprestore 16
	sw	$31,28($sp)
	.set	noreorder
	.set	nomacro
	beq	$4,$0,$L3
	sw	$28,24($sp)
	.set	macro
	.set	reorder

	move	$25,$4
	jal	$31,$25
$L3:
	lw	$31,28($sp)
	#nop
	.set	noreorder
	.set	nomacro
	j	$31
	addu	$sp,$sp,32
	.set	macro
	.set	reorder

	.end	dummy
$Lfe1:
	.size	dummy,$Lfe1-dummy
 #APP
	
/*@_init_PROLOG_BEGINS*/
	.section .init
 #NO_APP
	.align	2
	.globl	_init
	.ent	_init
_init:
	.frame	$sp,32,$31		# vars= 0, regs= 2/0, args= 16, extra= 8
	.mask	0x90000000,-4
	.fmask	0x00000000,0
	.set	noreorder
	.cpload	$25
	.set	reorder
	subu	$sp,$sp,32
	.cprestore 16
	sw	$31,28($sp)
	sw	$28,24($sp)
 #APP
	
/*@_init_PROLOG_PAUSES*/
 #NO_APP
	la	$25,i_am_not_a_leaf
	jal	$31,$25
 #APP
	
/*@_init_PROLOG_UNPAUSES*/
	ALIGN
	END_INIT
	
/*@_init_PROLOG_ENDS*/
	
/*@_init_EPILOG_BEGINS*/
 #NO_APP
	lw	$31,28($sp)
	#nop
	.set	noreorder
	.set	nomacro
	j	$31
	addu	$sp,$sp,32
	.set	macro
	.set	reorder

	.end	_init
$Lfe2:
	.size	_init,$Lfe2-_init
 #APP
	
/*@_init_EPILOG_ENDS*/
	
/*@_fini_PROLOG_BEGINS*/
	.section .fini
 #NO_APP
	.align	2
	.globl	_fini
	.ent	_fini
_fini:
	.frame	$sp,32,$31		# vars= 0, regs= 2/0, args= 16, extra= 8
	.mask	0x90000000,-4
	.fmask	0x00000000,0
	.set	noreorder
	.cpload	$25
	.set	reorder
	subu	$sp,$sp,32
	.cprestore 16
	sw	$31,28($sp)
	sw	$28,24($sp)
 #APP
	ALIGN
	END_FINI
	
/*@_fini_PROLOG_ENDS*/
 #NO_APP
	la	$25,i_am_not_a_leaf2
	jal	$31,$25
 #APP
	
/*@_fini_EPILOG_BEGINS*/
 #NO_APP
	lw	$31,28($sp)
	#nop
	.set	noreorder
	.set	nomacro
	j	$31
	addu	$sp,$sp,32
	.set	macro
	.set	reorder

	.end	_fini
$Lfe3:
	.size	_fini,$Lfe3-_fini
 #APP
	
/*@_fini_EPILOG_ENDS*/
	
/*@TRAILER_BEGINS*/
	.ident	"GCC: (GNU) 3.0 20010422 (prerelease) with bcm4710a0 modifications"
