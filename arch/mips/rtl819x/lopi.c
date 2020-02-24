// SPDX-License-Identifier: GPL-2.0-only

// Low-Overhead Prioritized Interrupts

//#define DEBUG
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/kernel.h>
#include <linux/irq.h>
#include <linux/irqchip.h>
#include <linux/irqdomain.h>
#include <rlx-cpu.h>

#define LOPI_IRQ_BASE   8
#define LOPI_IRQ_COUNT  8
#define EST0_IM         0x00ff0000
#define ECAUSEF_IP      0x00ff0000

static struct irq_domain *irq_domain;

static inline void lopi_unmask(struct irq_data *d)
{
	set_lxc0_estatus(IE_SW0 << d->hwirq);
	irq_enable_hazard();
}

static inline void lopi_mask(struct irq_data *d)
{
	clear_lxc0_estatus(IE_SW0 << d->hwirq);
	irq_disable_hazard();
}

static struct irq_chip irq_chip = {
	.name		= "LOPI",
	.irq_ack	= lopi_mask,
	.irq_mask	= lopi_mask,
	.irq_mask_ack	= lopi_mask,
	.irq_unmask	= lopi_unmask,
	.irq_eoi	= lopi_unmask,
	.irq_disable	= lopi_mask,
	.irq_enable	= lopi_unmask,
};

asmlinkage void __weak lopi_irq_dispatch(u32 irq)
{
	do_IRQ(irq);
}

static int lopi_map(struct irq_domain *d, unsigned int irq, irq_hw_number_t hw)
{
	pr_debug("lopi_map(%d)\n", irq);
	irq_set_chip_and_handler(irq, &irq_chip, handle_percpu_irq);
	return 0;
}

static const struct irq_domain_ops irq_domain_ops = {
	.map = lopi_map,
};

extern struct {} lopi_vectors;

int __init lopi_init(struct device_node *node, struct device_node *parent)
{
	clear_lxc0_estatus(EST0_IM);
	clear_lxc0_ecause(ECAUSEF_IP);
	irq_domain = irq_domain_add_legacy(node, LOPI_IRQ_COUNT, LOPI_IRQ_BASE, LOPI_IRQ_BASE, &irq_domain_ops, NULL);
	if (!irq_domain)
		panic("LOPI: Failed to add IRQ domain");

	irq_domain->name = "lexra-lopi-domain";

	write_lxc0_intvec(&lopi_vectors);
	pr_info("LOPI started\n");

	return 0;
}

IRQCHIP_DECLARE(lopi_intc, "lexra,lopi", lopi_init);
