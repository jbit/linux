#ifndef _MACH_RLX_CPU_H_
#define _MACH_RLX_CPU_H_

#include <asm/inst.h>
#include <asm/mipsregs.h>

// Instructions to read/write LXC0 registers
_ASM_MACRO_2R(mflxc0, rt, rd, _ASM_INSN_IF_MIPS(0x40600000 | __rt << 16 | __rd << 11));
_ASM_MACRO_2R(mtlxc0, rt, rd, _ASM_INSN_IF_MIPS(0x40e00000 | __rt << 16 | __rd << 11));

#define __read_32bit_lxc0_register(register) \
({ \
	unsigned int __res; \
	__asm__ __volatile__("mflxc0\t%0, " #register "\n\t" : "=r" (__res)); \
	__res; \
})

#define __write_32bit_lxc0_register(register, value) \
do { \
	__asm__ __volatile__("mtlxc0\t%z0, " #register "\n\t" : : "Jr" ((unsigned int)(value))); \
} while (0)


#define read_lxc0_estatus()     __read_32bit_lxc0_register($0)
#define write_lxc0_estatus(val) __write_32bit_lxc0_register($0, val)

#define read_lxc0_ecause()      __read_32bit_lxc0_register($1)
#define write_lxc0_ecause(val)  __write_32bit_lxc0_register($2, val)

#define read_lxc0_intvec()      __read_32bit_lxc0_register($3)
#define write_lxc0_intvec(val)  __write_32bit_lxc0_register($3, val)

__BUILD_SET_COMMON(lxc0_estatus)
__BUILD_SET_COMMON(lxc0_ecause)
__BUILD_SET_COMMON(lxc0_intvec)

#endif