// SPDX-License-Identifier: GPL-2.0-only

//#define DEBUG
#include <linux/clk-provider.h>
#include <linux/irqchip.h>
#include <linux/of.h>
#include <linux/of_fdt.h>
#include <linux/of_irq.h>
#include <asm/bootinfo.h>
#include <asm/time.h>
#include <asm/irq.h>
#include <asm/wbflush.h>
#include <asm/reboot.h>
#include <asm/irq_cpu.h>
#include <asm/prom.h>
#include <rtl819x-soc.h>

static void rtl_wbflush(void)
{
}

void (*__wbflush)(void) = rtl_wbflush;

void __init wbflush_setup(void)
{
}

const char *get_system_type(void)
{
	u32 id  = RTL819X_SOC_REV & 0xffffff00;
	u32 rev = RTL819X_SOC_REV & 0xff;
	const char *soc = NULL;
	static char buf[128];
	switch(id) {
	case SOC_ID_RTL8196C: soc = "RTL8196C"; break;
	case SOC_ID_RTL8198:  soc = "RTL8198";  break;
	case SOC_ID_RTL8196E: soc = "RTL8196E"; break;
	case SOC_ID_RTL8197D: soc = "RTL8197D"; break;
	case SOC_ID_RTL8198C: soc = "RTL8198C"; break;
	case SOC_ID_RTL8881A: soc = "RTL8881A"; break;
	}
	if (soc)
		snprintf(buf, sizeof(buf), "%s (Rev.%d)", soc, rev);
	else
		snprintf(buf, sizeof(buf), "Unknown 0x%06x (Rev.%d)", id, rev);
	return buf;
}

void __init prom_init(void)
{
	setup_8250_early_printk_port(RTL819X_SOC_UART0, 2, 0);
	pr_info("RTL819X: SoC is %s\n", get_system_type());
}

void __init prom_free_prom_memory(void)
{
}

static void rtl819x_add_memory(void) {
	u32 dcr = RTL819X_SOC_DCR;
	// Read the DRAM setup from the memory controller to figure out how much ram we have
	u32 buswidth, chipsel, rows, cols, banks, memsize;
	u32 version;

	switch (RTL819X_SOC_REV & 0xffffff00) {
	default:
		pr_info("RTL819X: DRAM: Assuming v1 DRAM controller\n");
		/* fallthrough */
	case SOC_ID_RTL8198:
	{
		struct rtl819x_soc_dcrv1 dcrv1 = *(struct rtl819x_soc_dcrv1*)&dcr;
		buswidth = 1    << dcrv1.buswidth;
		chipsel  = 1    << dcrv1.chipsels;
		rows     = 2048 << dcrv1.rows;
		cols     = 256  << dcrv1.cols;
		banks    = 2    << dcrv1.banks;
		memsize  = buswidth * chipsel * rows * cols * banks;
		version  = 1;
		break;
	}
	case SOC_ID_RTL8881A:
	{
		struct rtl819x_soc_dcrv2 dcrv2 = *(struct rtl819x_soc_dcrv2*)&dcr;
		buswidth = 1    << dcrv2.buswidth;
		chipsel  = 1    << dcrv2.chipsels;
		rows     = 2048 << dcrv2.rows;
		cols     = 256  << dcrv2.cols;
		banks    = 2    << dcrv2.banks;
		memsize  = buswidth * chipsel * rows * cols * banks;
		version  = 2;
		break;
	}
	}

	pr_info("RTL819X: DRAM: %dMiB DCRv%d(%08x) (bits=%d chips=%d rows=%d cols=%d banks=%d)\n", memsize/1024/1024, version, dcr, buswidth * 8, chipsel, rows, cols, banks);

	add_memory_region(RTL819X_SOC_DRAM, memsize, BOOT_MEM_RAM);
}

static void rtl819x_wdtreset(char*foo) {
	pr_info("Restarting...\n");
	// Enable watchdog and wait
	*(volatile u32*)(0xb800311c) = 0;
	while(1);
}

void __init plat_mem_setup(void)
{
	void *dtb;
	if (fw_passed_dtb) {
		pr_info("RTL819X: Using firmware device tree @%p\n", &fw_passed_dtb);
		dtb = (void *)fw_passed_dtb;
	} else if (__dtb_start != __dtb_end) {
		pr_info("RTL819X: Using built-in device tree\n");
		dtb = (void *)__dtb_start;
	} else {
		panic("No device tree available");
	}

	__dt_setup_arch(dtb);
	rtl819x_add_memory();

	_machine_restart = rtl819x_wdtreset;
}

void __init device_tree_init(void)
{
	pr_debug("RTL819X: device_tree_init\n");
	unflatten_and_copy_device_tree();
}

void __init plat_time_init(void)
{
	pr_debug("RTL819X: plat_time_init\n");
	of_clk_init(NULL);
	timer_probe();
}

int __init lopi_init(struct device_node *of_node, struct device_node *parent);
int __init rtl819x_soc_intc_init(struct device_node *of_node, struct device_node *parent);

void __init arch_init_irq(void)
{
	pr_debug("RTL819X: arch_init_irq\n");	
	irqchip_init();
}
