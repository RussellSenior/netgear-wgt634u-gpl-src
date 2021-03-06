/*
 *  arch/s390/kernel/time.c
 *
 *  S390 version
 *    Copyright (C) 1999 IBM Deutschland Entwicklung GmbH, IBM Corporation
 *    Author(s): Hartmut Penner (hp@de.ibm.com),
 *               Martin Schwidefsky (schwidefsky@de.ibm.com),
 *               Denis Joseph Barrow (djbarrow@de.ibm.com,barrow_dj@yahoo.com)
 *
 *  Derived from "arch/i386/kernel/time.c"
 *    Copyright (C) 1991, 1992, 1995  Linus Torvalds
 */

#include <linux/errno.h>
#include <linux/sched.h>
#include <linux/kernel.h>
#include <linux/param.h>
#include <linux/string.h>
#include <linux/mm.h>
#include <linux/interrupt.h>
#include <linux/time.h>
#include <linux/delay.h>
#include <linux/init.h>
#include <linux/smp.h>
#include <linux/types.h>

#include <asm/uaccess.h>
#include <asm/delay.h>

#include <linux/timex.h>
#include <linux/config.h>

#include <asm/irq.h>
#include <asm/s390_ext.h>

/* change this if you have some constant time drift */
#define USECS_PER_JIFFY     ((unsigned long) 1000000/HZ)
#define CLK_TICKS_PER_JIFFY ((unsigned long) USECS_PER_JIFFY << 12)

#define TICK_SIZE tick

static ext_int_info_t ext_int_info_timer;
static uint64_t init_timer_cc;

extern rwlock_t xtime_lock;
extern unsigned long wall_jiffies;

void tod_to_timeval(__u64 todval, struct timeval *xtime)
{
        todval >>= 12;
        xtime->tv_sec = todval / 1000000;
        xtime->tv_usec = todval % 1000000;
}

static inline unsigned long do_gettimeoffset(void) 
{
	__u64 now;

	asm ("STCK 0(%0)" : : "a" (&now) : "memory", "cc");
        now = (now - init_timer_cc) >> 12;
	/* We require the offset from the latest update of xtime */
	now -= (__u64) wall_jiffies*USECS_PER_JIFFY;
	return (unsigned long) now;
}

/*
 * This version of gettimeofday has microsecond resolution.
 */
void do_gettimeofday(struct timeval *tv)
{
	unsigned long flags;
	unsigned long usec, sec;

	read_lock_irqsave(&xtime_lock, flags);
	sec = xtime.tv_sec;
	usec = xtime.tv_usec + do_gettimeoffset();
	read_unlock_irqrestore(&xtime_lock, flags);

	while (usec >= 1000000) {
		usec -= 1000000;
		sec++;
	}

	tv->tv_sec = sec;
	tv->tv_usec = usec;
}

void do_settimeofday(struct timeval *tv)
{

	write_lock_irq(&xtime_lock);
	/* This is revolting. We need to set the xtime.tv_usec
	 * correctly. However, the value in this location is
	 * is value at the last tick.
	 * Discover what correction gettimeofday
	 * would have done, and then undo it!
	 */
	tv->tv_usec -= do_gettimeoffset();

	while (tv->tv_usec < 0) {
		tv->tv_usec += 1000000;
		tv->tv_sec--;
	}

	xtime = *tv;
	time_adjust = 0;		/* stop active adjtime() */
	time_status |= STA_UNSYNC;
	time_maxerror = NTP_PHASE_LIMIT;
	time_esterror = NTP_PHASE_LIMIT;
	write_unlock_irq(&xtime_lock);
}

/*
 * timer_interrupt() needs to keep up the real-time clock,
 * as well as call the "do_timer()" routine every clocktick
 */

#ifdef CONFIG_SMP
extern __u16 boot_cpu_addr;
#endif

static void do_comparator_interrupt(struct pt_regs *regs, __u16 error_code)
{
	int cpu = smp_processor_id();

	irq_enter(cpu, 0);

	/*
	 * set clock comparator for next tick
	 */
        S390_lowcore.jiffy_timer += CLK_TICKS_PER_JIFFY;
        asm volatile ("SCKC %0" : : "m" (S390_lowcore.jiffy_timer));

#ifdef CONFIG_SMP
	if (S390_lowcore.cpu_data.cpu_addr == boot_cpu_addr)
		write_lock(&xtime_lock);

	update_process_times(user_mode(regs));

	if (S390_lowcore.cpu_data.cpu_addr == boot_cpu_addr) {
		do_timer(regs);
		write_unlock(&xtime_lock);
	}
#else
	do_timer(regs);
#endif

	irq_exit(cpu, 0);
}

/*
 * Start the clock comparator on the current CPU
 */
void init_cpu_timer(void)
{
	unsigned long cr0;

        /* allow clock comparator timer interrupt */
        asm volatile ("STCTG 0,0,%0" : "=m" (cr0) : : "memory");
        cr0 |= 0x800;
        asm volatile ("LCTLG 0,0,%0" : : "m" (cr0) : "memory");
	S390_lowcore.jiffy_timer = (__u64) jiffies * CLK_TICKS_PER_JIFFY;
	S390_lowcore.jiffy_timer += init_timer_cc + CLK_TICKS_PER_JIFFY;
	asm volatile ("SCKC %0" : : "m" (S390_lowcore.jiffy_timer));
}

/*
 * Initialize the TOD clock and the CPU timer of
 * the boot cpu.
 */
void __init time_init(void)
{
        __u64 set_time_cc;
	int cc;

        /* kick the TOD clock */
        asm volatile ("STCK 0(%1)\n\t"
                      "IPM  %0\n\t"
                      "SRL  %0,28" : "=r" (cc) : "a" (&init_timer_cc) 
				   : "memory", "cc");
        switch (cc) {
        case 0: /* clock in set state: all is fine */
                break;
        case 1: 
                printk("time_init: TOD clock in non-set state\n");
                break;
        case 2: 
                printk("time_init: TOD clock in error state\n");
                break;
        case 3: 
                printk("time_init: TOD clock stopped/non-operational\n");
                break;
        }

	/* set xtime */
        set_time_cc = init_timer_cc - 0x8126d60e46000000LL +
                      (0x3c26700LL*1000000*4096);
        tod_to_timeval(set_time_cc, &xtime);

        /* request the 0x1004 external interrupt */
        if (register_early_external_interrupt(0x1004, do_comparator_interrupt,
					      &ext_int_info_timer) != 0)
                panic("Couldn't request external interrupt 0x1004");

        /* init CPU timer */
        init_cpu_timer();
}
