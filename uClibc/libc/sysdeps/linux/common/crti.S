	.file	1 "initfini.c"
	.abicalls
 #APP
	
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
	
	.align 2
	.end _init
	
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
	.align 2
	.end _fini
	
	.ident	"GCC: (GNU) 3.0 20010422 (prerelease) with bcm4710a0 modifications"
