// SPDX-License-Identifier: GPL-2.0-only
//#define DEBUG
#include <linux/clocksource.h>
#include <linux/clockchips.h>
#include <linux/cpu.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/io.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/sched_clock.h>
#include <asm/delay.h>

// TCCND values
#define TCCNR_TC0EN     (1 << 31)
#define TCCNR_TC0TIMER  (1 << 30)
#define TCCNR_TC1EN     (1 << 29)
#define TCCNR_TC1TIMER  (1 << 28)

// TCIR values
#define TCIR_TC0IE      (1 << 31)
#define TCIR_TC1IE      (1 << 30)
#define TCIR_TC0IP      (1 << 29)
#define TCIR_TC1IP      (1 << 28)

// CDBR values
#define CDBR_SHIFT      16

#define REG_TC0DATA 0x00 // 0x18003100 Timer/Counter 0 Data
#define REG_TC1DATA 0x04 // 0x18003104 Timer/Counter 1 Data
#define REG_TC0CNT  0x08 // 0x18003108 Timer/Counter 0 Count
#define REG_TC1CNT  0x0C // 0x1800310C Timer/Counter 1 Count
#define REG_TCCNR   0x10 // 0x18003110 Timer/Counter Control
#define REG_TCIR    0x14 // 0x18003114 Timer/Counter Interrupt
#define REG_CDBR    0x18 // 0x18003118 Clock Division
#define REG_WDTCNR  0x1C // 0x1800311C Watchdog Timer Control

// Timer0 clocksource
struct rtl819x_clocksource {
	struct clocksource cs;
	void __iomem *reg_cnt;
	u32 shift;
};

static u64 rtl819x_soc_timer_clocksource_read(struct clocksource *cs)
{
	struct rtl819x_clocksource *rcs = container_of(cs, struct rtl819x_clocksource, cs);
	return readl(rcs->reg_cnt) >> rcs->shift;
}

// Timer0 clock event
struct rtl819x_clock_event_device {
	struct clock_event_device ced;
	void __iomem *base;
	void __iomem *reg_data;
	u32 periodic;
	u32 shift;
	u32 tcir_mask;
	u32 tccnr_bit;
	u32 tcir_bit;
};

static void rtl819x_soc_timer_start(struct rtl819x_clock_event_device *rced)
{
	void __iomem *tccnr = rced->base + REG_TCCNR;
	void __iomem *tcir = rced->base + REG_TCIR;

	if ((u32)tccnr != 0xB8003110) tccnr = 0;
	if ((u32)tcir != 0xB8003114) tcir = 0;

	writel(readl(tccnr) | rced->tccnr_bit, tccnr);
	writel(readl(tcir) | rced->tcir_bit, tcir);
}

static void rtl819x_soc_timer_stop(struct rtl819x_clock_event_device *rced)
{
	void __iomem *tccnr = rced->base + REG_TCCNR;
	void __iomem *tcir = rced->base + REG_TCIR;

	if ((u32)tccnr != 0xB8003110) tccnr = 0;
	if ((u32)tcir != 0xB8003114) tcir = 0;

	writel(readl(tccnr) & ~rced->tccnr_bit, tccnr);
	writel(readl(tcir) & ~rced->tcir_bit, tcir);
}

static void rtl819x_soc_timer_set(struct rtl819x_clock_event_device *rced, u32 next)
{
	rtl819x_soc_timer_stop(rced);
	if ((u32)rced->reg_data != 0xB8003100) rced->reg_data = 0;
	writel(next << rced->shift, rced->reg_data);
	rtl819x_soc_timer_start(rced);
}

static int rtl819x_soc_timer_next_event(unsigned long delta, struct clock_event_device *ced)
{
	struct rtl819x_clock_event_device *rced = container_of(ced, struct rtl819x_clock_event_device, ced);
	rtl819x_soc_timer_set(rced, delta);
	return 0;
}

static int rtl819x_soc_timer_periodic(struct clock_event_device *ced)
{
	struct rtl819x_clock_event_device *rced = container_of(ced, struct rtl819x_clock_event_device, ced);
	rtl819x_soc_timer_set(rced, rced->periodic);
	return 0;
}

static int rtl819x_soc_timer_oneshot(struct clock_event_device *ced)
{
	struct rtl819x_clock_event_device *rced = container_of(ced, struct rtl819x_clock_event_device, ced);
	rtl819x_soc_timer_stop(rced);
	return 0;
}

static int rtl819x_soc_timer_shutdown(struct clock_event_device *ced)
{
	struct rtl819x_clock_event_device *rced = container_of(ced, struct rtl819x_clock_event_device, ced);
	rtl819x_soc_timer_stop(rced);
	return 0;
}

static int rtl819x_soc_timer_resume(struct clock_event_device *ced)
{
	struct rtl819x_clock_event_device *rced = container_of(ced, struct rtl819x_clock_event_device, ced);
	rtl819x_soc_timer_stop(rced);
	return 0;
}

static irqreturn_t rtl819x_soc_timer_handler(int irq, void *dev_id)
{
	struct rtl819x_clock_event_device *rced = dev_id;
	if (!rced)
		return IRQ_NONE;

	// Remove pending flag only for our timer
	writel(readl(rced->base + REG_TCIR) & rced->tcir_mask, rced->base + REG_TCIR);

	if (!clockevent_state_periodic(&rced->ced))
		rtl819x_soc_timer_shutdown(&rced->ced);

	if (rced->ced.event_handler)
		rced->ced.event_handler(&rced->ced);


	return IRQ_HANDLED;
}

static struct clocksource *sched_clocksource = NULL;
static u64 notrace rtl819x_soc_sched_clock_read(void)
{
	if (sched_clocksource)
		return sched_clocksource->read(sched_clocksource);
	else
		return 0;
}

static int __init rtl819x_soc_timer_init(struct device_node *node)
{
	u32 clkfrq = 0;
	u32 clkdiv = 0;
	u32 shift = 0;
	int timer0_irq = irq_of_parse_and_map(node, 0);
	int timer1_irq = irq_of_parse_and_map(node, 1);
	void *base = of_io_request_and_map(node, 0, NULL);
	struct rtl819x_clocksource *rcs0 = kzalloc(sizeof(struct rtl819x_clocksource), GFP_KERNEL);
	struct rtl819x_clocksource *rcs1 = kzalloc(sizeof(struct rtl819x_clocksource), GFP_KERNEL);
	struct rtl819x_clock_event_device *rced = kzalloc(sizeof(struct rtl819x_clock_event_device), GFP_KERNEL);

	if (!timer0_irq)
		panic("RTL819X timer0: Failed to get IRQ");

	if (!timer1_irq)
		panic("RTL819X timer1: Failed to get IRQ");

	if (!base)
		panic("RTL819X timer: Failed to get MMIO");

	if (!rcs0 || !rcs1 || !rced)
		panic("RTL819X timer: Failed to allocate memory");

	if (of_property_read_u32(node, "clock-frequency", &clkfrq))
		panic("RTL819X timer: Failed to get clock-frequency");

	if (of_property_read_u32(node, "clock-div", &clkdiv))
		panic("RTL819X timer: Failed to get clock-div");

	if (of_property_read_u32(node, "shift", &shift))
		panic("RTL819X timer: Failed to get shift");

	// Disable counters
	writel(0, base + REG_TCCNR);

	// Clear interrupts
	writel(TCIR_TC0IP | TCIR_TC1IP, base + REG_TCIR);

	// Disable watchdog
	writel((0xA5 << 24) | (1 << 23) | (1 << 20), base + REG_WDTCNR);

	// Setup division
	writel(clkdiv << CDBR_SHIFT, base + REG_CDBR);

	clkfrq /= clkdiv;

	// Setup clocksource0
	rcs0->shift     = shift;
	rcs0->reg_cnt   = base + REG_TC0CNT;
	rcs0->cs.name   = "rtl819x_soc_timer0";
	rcs0->cs.mask   = CLOCKSOURCE_MASK(32 - shift);
	rcs0->cs.read   = rtl819x_soc_timer_clocksource_read;
	rcs0->cs.flags  = 0;
	rcs0->cs.rating = 100;

	if (clocksource_register_hz(&rcs0->cs, clkfrq))
		panic("RTL819X timer0: Failed to register clocksource");

	// Setup clocksource1
	rcs1->shift     = shift;
	rcs1->reg_cnt   = base + REG_TC1CNT;
	rcs1->cs.name   = "rtl819x_soc_timer1";
	rcs1->cs.mask   = CLOCKSOURCE_MASK(32 - shift);
	rcs1->cs.read   = rtl819x_soc_timer_clocksource_read;
	rcs1->cs.flags  = CLOCK_SOURCE_IS_CONTINUOUS;
	rcs1->cs.rating = 300;

	// Start timer1 in freerunning mode, no interrupts
	writel(0xffffffff, base + REG_TC1DATA);
	writel(TCCNR_TC1EN | TCCNR_TC1TIMER, base + REG_TCCNR);

	if (clocksource_register_hz(&rcs1->cs, clkfrq))
		panic("RTL819X timer1: Failed to register clocksource");

	if (!sched_clocksource) {
		sched_clocksource = &rcs1->cs;
		sched_clock_register(rtl819x_soc_sched_clock_read, 32 - shift, clkfrq);
	}

	// Clock event / interrupt setup
	rced->shift     = shift;
	rced->periodic  = clkfrq / HZ;
	rced->base      = base;
	rced->reg_data  = base + REG_TC0DATA;
	rced->tcir_mask = TCIR_TC0IE | TCIR_TC1IE | TCIR_TC0IP;
	rced->tccnr_bit = TCCNR_TC0EN | TCCNR_TC0TIMER;
	rced->tcir_bit  = TCIR_TC0IE;
	rced->ced.name                  = "rtl819x_soc_timer0";
	rced->ced.features              = CLOCK_EVT_FEAT_PERIODIC | CLOCK_EVT_FEAT_ONESHOT;
	rced->ced.rating                = 100;
	rced->ced.irq                   = timer0_irq;
	rced->ced.set_next_event        = rtl819x_soc_timer_next_event;
	rced->ced.set_state_periodic    = rtl819x_soc_timer_periodic;
	rced->ced.set_state_oneshot     = rtl819x_soc_timer_oneshot;
	rced->ced.set_state_shutdown    = rtl819x_soc_timer_shutdown;
	rced->ced.tick_resume           = rtl819x_soc_timer_resume;
	if (request_irq(rced->ced.irq, rtl819x_soc_timer_handler, IRQF_TIMER, "rtl819x_soc_timer0", rced))
		panic("RTL819X timer0: Failed to register IRQ handler");
	clockevents_config_and_register(&rced->ced, clkfrq, 1, LONG_MAX);

	pr_info("RTL819x timers started (%dMHz)\n", clkfrq/1000/1000);

	return 0;
}

TIMER_OF_DECLARE(rtl819x_soc_timer, "realtek,rtl819x-soc-timer", rtl819x_soc_timer_init);
