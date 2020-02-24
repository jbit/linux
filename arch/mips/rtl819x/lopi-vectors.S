/* Lexra Low-Overhead Prioritized Interrupts */

#include <linux/init.h>
#include <asm/asm.h>
#include <asm/asmmacro.h>
#include <asm/regdef.h>
#include <asm/stackframe.h>

__INIT

.macro	LOPI_VECTOR number
lopi_vector\number:
	j lopi_handler
	addiu a0, zero, \number
	.size lopi_vector\number, .-lopi_vector\number
.endm


.align	6
.globl lopi_vectors
lopi_vectors:
	.set push
	.set noat
	.set noreorder
	LOPI_VECTOR 8
	LOPI_VECTOR 9
	LOPI_VECTOR 10
	LOPI_VECTOR 11
	LOPI_VECTOR 12
	LOPI_VECTOR 13
	LOPI_VECTOR 14
	LOPI_VECTOR 15
	.set pop
	.size lopi_vectors, .-lopi_vectors

lopi_handler:
	.set push
	.set noat
	SAVE_ALL
	LONG_L s0, TI_REGS($28)
	LONG_S sp, TI_REGS($28)
	PTR_LA ra, ret_from_irq
	j lopi_irq_dispatch
	.set pop
	.size lopi_handler, .-lopi_handler
