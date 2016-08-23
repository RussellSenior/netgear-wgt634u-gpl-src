	.file	1 "initfini.c"
	.abicalls
 #APP
	
	.section .init
 #NO_APP
	.align	2
	.globl	_init
	.ent	_init
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
	
	.section .fini
 #NO_APP
	.align	2
	.globl	_fini
	.ent	_fini
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
	
	.ident	"GCC: (GNU) 3.0 20010422 (prerelease) with bcm4710a0 modifications"
