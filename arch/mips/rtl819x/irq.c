// SPDX-License-Identifier: GPL-2.0-only
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/kernel.h>
#include <linux/irq.h>
#include <linux/irqchip.h>
#include <linux/irqchip/chained_irq.h>
#include <linux/irqdomain.h>
#include <linux/of_irq.h>
#include <linux/of_address.h>

#define IRQ_COUNT       32
#define REG_GIMR        0x00 // 0x18003000 - Interrupt Mask
#define REG_GISR        0x04 // 0x18003004 - Interrupt Status
#define REG_IRR0        0x08 // 0x18003008 - Interrupt Routing 0
#define REG_IRR1        0x0c // 0x1800300C - Interrupt Routing 1
#define REG_IRR2        0x10 // 0x18003010 - Interrupt Routing 2
#define REG_IRR3        0x14 // 0x18003014 - Interrupt Routing 3

struct rtl819x_chip_data {
	void __iomem *base;
	struct irq_chip *chip;
};

static inline void rtl819x_soc_intc_unmask(struct irq_data *data)
{
	irq_hw_number_t hwirq = irqd_to_hwirq(data);
	struct rtl819x_chip_data *cd = irq_data_get_irq_chip_data(data);
	void __iomem *gimr = cd->base + REG_GIMR;
	writel(readl(gimr) | BIT(hwirq), gimr);
}

static inline void rtl819x_soc_intc_mask(struct irq_data *data)
{
	irq_hw_number_t hwirq = irqd_to_hwirq(data);
	struct rtl819x_chip_data *cd = irq_data_get_irq_chip_data(data);
	void __iomem *gimr = cd->base + REG_GIMR;
	writel(readl(gimr) & ~BIT(hwirq), gimr);
}

static int rtl819x_soc_intc_map(struct irq_domain *irq_domain, unsigned int irq, irq_hw_number_t hw)
{
	struct rtl819x_chip_data *cd = irq_domain->host_data;
	irq_set_chip_and_handler(irq, cd->chip, handle_level_irq);
	irq_set_chip_data(irq, irq_domain->host_data);
	return 0;
}

static void rtl819x_soc_intc_handler(struct irq_desc *parent_desc)
{
	struct irq_domain *local_irq_domain = irq_desc_get_handler_data(parent_desc);
	struct rtl819x_chip_data *cd = local_irq_domain->host_data;
	u32 pending = readl(cd->base + REG_GISR) & readl(cd->base + REG_GIMR);
	if (pending) 
		generic_handle_irq(irq_find_mapping(local_irq_domain, __ffs(pending)));
	else
		spurious_interrupt();
}

static struct irq_chip irq_chip = {
	.name           = "rtl819x-soc-intc",
	.irq_ack        = rtl819x_soc_intc_mask,
	.irq_mask       = rtl819x_soc_intc_mask,
	.irq_mask_ack   = rtl819x_soc_intc_mask,
	.irq_unmask     = rtl819x_soc_intc_unmask,
	.irq_eoi        = rtl819x_soc_intc_unmask,
	.irq_disable    = rtl819x_soc_intc_mask,
	.irq_enable     = rtl819x_soc_intc_unmask,
};

static const struct irq_domain_ops irq_domain_ops = {
	.map = rtl819x_soc_intc_map,
};

int __init rtl819x_soc_intc_init(struct device_node *node, struct device_node *parent)
{
	u32 routing;
	int irq = irq_of_parse_and_map(node, 0);
	void *base = of_io_request_and_map(node, 0, NULL);
	struct rtl819x_chip_data *cd = kzalloc(sizeof(struct rtl819x_chip_data), GFP_KERNEL);
	struct irq_domain *irq_domain = irq_domain_add_linear(node, IRQ_COUNT, &irq_domain_ops, cd);

	if (!irq)
		panic("RTL819X INTC: Failed to get IRQ");
	
	if (!base)
		panic("RTL819X INTC: Failed to get MMIO");

	if (!cd)
		panic("RTL819X INTC: Failed to allocate chip data");

	if (!irq_domain)
		panic("RTL819X INTC: Failed to add IRQ domain");

	irq_domain->name = "rtl819x-soc-intc";
	cd->base = base;
	cd->chip = &irq_chip;

	routing = 
		irq << 0  | irq << 4  | irq << 8  | irq << 12 |
		irq << 16 | irq << 20 | irq << 24 | irq << 28;

	//pr_debug("rtl819x_soc_intc_handler %08x\n", (u32)(cd->base + REG_GIMR));

	writel(0,       cd->base + REG_GIMR);
	writel(routing, cd->base + REG_IRR0);
	writel(routing, cd->base + REG_IRR1);
	writel(routing, cd->base + REG_IRR2);
	writel(routing, cd->base + REG_IRR3);

	irq_set_chained_handler_and_data(irq, rtl819x_soc_intc_handler, irq_domain);

	pr_info("RTL819x INTC started (cascaded to IRQ%d)\n", irq);

	return 0;
}

IRQCHIP_DECLARE(rtl819x_soc_intc, "realtek,rtl819x-soc-intc", rtl819x_soc_intc_init);
