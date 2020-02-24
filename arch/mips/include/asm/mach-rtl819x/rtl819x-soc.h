#ifndef _MACH_RTL819X_SOC_H_
#define _MACH_RTL819X_SOC_H_

#include <linux/types.h>
#define RTL819X_SOC_DRAM    0x00000000
#define RTL819X_SOC_SYSTEM  0xb8000000
#define RTL819X_SOC_EFUSE   0xb8000700
#define RTL819X_SOC_MEMCTRL 0xb8001000
#define RTL819X_SOC_NFCR    0xb8001100
#define RTL819X_SOC_SPI     0xb8001200
#define RTL819X_SOC_UART0   0xb8002000
#define RTL819X_SOC_UART1   0xb8002100
#define RTL819X_SOC_INTC    0xb8003000
#define RTL819X_SOC_TIMER   0xb8003100
#define RTL819X_SOC_GPIO    0xb8003500
#define RTL819X_SOC_DMA2    0xb800b000
#define RTL819X_SOC_CRYPTO  0xb800c000
#define RTL819X_SOC_NFBI    0xb8019000
#define RTL819X_SOC_OHCI    0xb8020000
#define RTL819X_SOC_EHCI    0xb8021000
#define RTL819X_SOC_OCP     0xb8040000
#define RTL819X_SOC_PCIE    0xb8b00000
#define RTL819X_SOC_SWITCH  0xbb000000
#define RTL819X_SOC_FLASH   0xbd000000
#define RTL819X_SOC_BOOTROM 0xbfc00000
#define RTL819X_SOC_EJTAG   0xff220000


#define RTL819X_SOC_REG32(_addr) (*(volatile u32 *)(_addr))

// System registers
#define RTL819X_SOC_REV     RTL819X_SOC_REG32(RTL819X_SOC_SYSTEM|0x00)
#define RTL819X_SOC_STRAP   RTL819X_SOC_REG32(RTL819X_SOC_SYSTEM|0x08)
#define RTL819X_SOC_BOND    RTL819X_SOC_REG32(RTL819X_SOC_SYSTEM|0x0c)
#define RTL819X_SOC_CLK     RTL819X_SOC_REG32(RTL819X_SOC_SYSTEM|0x10)
#define RTL819X_SOC_PLL     RTL819X_SOC_REG32(RTL819X_SOC_SYSTEM|0x20)
#define RTL819X_SOC_PINMUX  RTL819X_SOC_REG32(RTL819X_SOC_SYSTEM|0x30)
#define RTL819X_SOC_PCIEPLL RTL819X_SOC_REG32(RTL819X_SOC_SYSTEM|0x44)


// top 24bits of RTL819X_SOC_REV
#define SOC_ID_RTL8196C         0x80000000
#define SOC_ID_RTL8196E         0x8196e000
#define SOC_ID_RTL8197D         0x8197c000
#define SOC_ID_RTL8198C         0x8198c000
#define SOC_ID_RTL8198          0xC0000000
#define SOC_ID_RTL8881A         0x8881a000

// Memory controller registers
#define RTL819X_SOC_MCR     RTL819X_SOC_REG32(RTL819X_SOC_MEMCTRL|0x00)
#define RTL819X_SOC_DCR     RTL819X_SOC_REG32(RTL819X_SOC_MEMCTRL|0x04)
#define RTL819X_SOC_DTR     RTL819X_SOC_REG32(RTL819X_SOC_MEMCTRL|0x08)
#define RTL819X_SOC_MPMR    RTL819X_SOC_REG32(RTL819X_SOC_MEMCTRL|0x40)
#define RTL819X_SOC_DDCR    RTL819X_SOC_REG32(RTL819X_SOC_MEMCTRL|0x50)
struct rtl819x_soc_dcrv1 {
	u32
		cas:            2,  // 30-31
		buswidth:       2,  // 28-29
		chipsels:       1,  // 27
		rows:           2,  // 25-26
		cols:           3,  // 22-24
		burstrefresh:   1,  // 21
		arbitration:    1,  // 20
		banks:          1,  // 19
		fastrx:         1,  // 18
		extmr:          1,  // 17
		drivestrength:  1,  // 16
		reserved:       16;
};

struct rtl819x_soc_dcrv2 {
	u32
		reserved1:      2,  // 30-31
		banks:          2,  // 28-29
		reserved0:      2,  // 26-27
		buswidth:       2,  // 24-25
		rows:           4,  // 20-23
		cols:           4,  // 16-19
		chipsels:       1,  // 15
		reserved:       15;
};

// SPI controller registers
#define RTL819X_SOC_SFCR    RTL819X_SOC_REG32(RTL819X_SOC_SPI|0x00)
#define RTL819X_SOC_SFCR2   RTL819X_SOC_REG32(RTL819X_SOC_SPI|0x04)
#define RTL819X_SOC_SFCSR   RTL819X_SOC_REG32(RTL819X_SOC_SPI|0x08)
#define RTL819X_SOC_SFDR    RTL819X_SOC_REG32(RTL819X_SOC_SPI|0x0C)
#define RTL819X_SOC_SFDR2   RTL819X_SOC_REG32(RTL819X_SOC_SPI|0x10)



#endif
