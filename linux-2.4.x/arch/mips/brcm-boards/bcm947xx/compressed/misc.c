/*
 * Misc initialization and support routines for self-booting
 * compressed image.
 *
 * Copyright 2001-2003, Broadcom Corporation
 * All Rights Reserved.
 * 
 * THIS SOFTWARE IS OFFERED "AS IS", AND BROADCOM GRANTS NO WARRANTIES OF ANY
 * KIND, EXPRESS OR IMPLIED, BY STATUTE, COMMUNICATION OR OTHERWISE. BROADCOM
 * SPECIFICALLY DISCLAIMS ANY IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A SPECIFIC PURPOSE OR NONINFRINGEMENT CONCERNING THIS SOFTWARE.
 *
 * $Id$
 */

#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/mm.h>
#include <linux/serial_reg.h>
#include <linux/serial.h>
#include <linux/delay.h>

#include <asm/bootinfo.h>
#include <asm/cpu.h>
#include <asm/bcache.h>
#include <asm/io.h>
#include <asm/page.h>
#include <asm/pgtable.h>
#include <asm/system.h>
#include <asm/mmu_context.h>

#include <typedefs.h>
#include <bcmdevs.h>
#include <bcmnvram.h>
#include <bcmutils.h>
#include <sbconfig.h>
#include <sbextif.h>
#include <sbchipc.h>
#include <sbmips.h>
#include <sbmemc.h>
#include <sflash.h>

/* At 125 MHz */
unsigned long loops_per_jiffy = 625000;

/* Static variables */
static unsigned int chipid, chiprev, mipscore;
static unsigned int sbclock, mipsclock;
static extifregs_t *eir;
static chipcregs_t *cc;
static mipsregs_t *mipsr;
static sbmemcregs_t *memc;
static void *usb;
static struct serial_struct uart;
static struct sflash *sflash;

#define LOG_BUF_LEN	(1024)
#define LOG_BUF_MASK	(LOG_BUF_LEN-1)
static char log_buf[LOG_BUF_LEN];
static unsigned long log_start;

/* Declarations needed for the cache related includes below */

/* Primary cache parameters. These declarations are needed*/
static int icache_size, dcache_size;	/* Size in bytes */
static int ic_lsize, dc_lsize;		/* LineSize in bytes */

/* Chip information */
unsigned int bcm_chipid = BCM4710_DEVICE_ID;
unsigned int bcm_chiprev = 0;

#if 0 /* fix latter ... hope cfe has done it */
#include <asm/cacheops.h>
#include <asm/bcm4710_cache.h>

__BUILD_SET_C0(taglo,CP0_TAGLO);
__BUILD_SET_C0(taghi,CP0_TAGHI);

static void
cache_init(void)
{
	unsigned int config1;
	unsigned int sets, ways;
	unsigned int start, end;

	config1 = read_c0_config1(); 

	/* Instruction Cache Size = Associativity * Line Size * Sets Per Way */
	if ((ic_lsize = ((config1 >> 19) & 7)))
		ic_lsize = 2 << ic_lsize;
	sets = 64 << ((config1 >> 22) & 7);
	ways = 1 + ((config1 >> 16) & 7);
	icache_size = ic_lsize * sets * ways;

	start = KSEG0;
	end = (start + icache_size);
	clear_c0_taglo(~0);
	clear_c0_taghi(~0);
	while (start < end) {
		cache_unroll(start, Index_Store_Tag_I);
		start += ic_lsize;
	}

	/* Data Cache Size = Associativity * Line Size * Sets Per Way */
	if ((dc_lsize = ((config1 >> 10) & 7)))
		dc_lsize = 2 << dc_lsize;
	sets = 64 << ((config1 >> 13) & 7);
	ways = 1 + ((config1 >> 7) & 7);
	dcache_size = dc_lsize * sets * ways;

	start = KSEG0;
	end = (start + dcache_size);
	clear_c0_taglo(~0);
	clear_c0_taghi(~0);
	while (start < end) {
		cache_unroll(start, Index_Store_Tag_D);
		start += dc_lsize;
	}
}
#endif

static inline unsigned int
serial_in(struct serial_struct *info, int offset)
{
#ifdef CONFIG_BCM4310
	readb((unsigned long) info->iomem_base +
	      (UART_SCR<<info->iomem_reg_shift));
#endif
	return readb((unsigned long) info->iomem_base +
		     (offset<<info->iomem_reg_shift));
}

static inline void
serial_out(struct serial_struct *info, int offset, int value)
{
#ifdef SIM
	return;
#else
	writeb(value, (unsigned long) info->iomem_base +
	       (offset<<info->iomem_reg_shift));
#endif
}

static void
sb_scan(void)
{
	int i;
	unsigned long cid, regs;
	sbconfig_t *sb;

	/* Initialize static variables */
	eir = NULL;
	cc = NULL;
	usb = NULL;
	memc = NULL;
	mipsr = NULL;
	mipscore = 0;
	chipid = BCM4710_DEVICE_ID;
	chiprev = 0;

	/* Too early to probe or malloc */
	for (i = 0; i < SB_MAXCORES; i++) {
		regs = SB_ENUM_BASE + (i * SB_CORE_SIZE);
		sb = (sbconfig_t *) KSEG1ADDR(regs + SBCONFIGOFF);
		cid  = (readl(&sb->sbidhigh) & SBIDH_CC_MASK) >> SBIDH_CC_SHIFT;
		switch (cid) {
		case SB_EXTIF:
			eir = (extifregs_t *) KSEG1ADDR(regs);
			break;
		case SB_CC:
			cc = (chipcregs_t *) KSEG1ADDR(regs);
			chipid = readl(&cc->chipid) & CID_ID_MASK;
			chiprev = (readl(&cc->chipid) & CID_REV_MASK) >> CID_REV_SHIFT;
			break;
		case SB_USB:
			usb = (void *)KSEG1ADDR(regs);
			break;
		case SB_MEMC:
			memc = (void *)KSEG1ADDR(regs);
			break;
		case SB_MIPS:
		case SB_MIPS33:
			mipsr = (void *)KSEG1ADDR(regs);
			mipscore = cid;
			break;
		}
		if (eir)
			break;
		if (cc && mipsr) {
			if (chipid == BCM4310_DEVICE_ID && chiprev == 0 && !usb)
				continue;
			else if (!memc)
				continue;
			break;
		}
	}
}

static int
keyhit(void)
{
#ifdef SIM
	return(1);
#endif

	return ((serial_in(&uart, UART_LSR) & UART_LSR_DR) != 0);
}

static int
getc(void)
{
#ifdef SIM
	return(0);
#endif

	while (!(serial_in(&uart, UART_LSR) & UART_LSR_DR));
	return (serial_in(&uart, UART_RX));
}

static void
putc(int c)
{
#ifdef SIM
	return;
#endif
	/* CR before LF */
	if (c == '\n')
		putc('\r');

	/* Store in log buffer */
	*((char *) KSEG1ADDR(&log_buf[log_start])) = (char) c;
	log_start = (log_start + 1) & LOG_BUF_MASK;

	while (!(serial_in(&uart, UART_LSR) & UART_LSR_THRE));
	serial_out(&uart, UART_TX, c);
}

static void
puts(const char *cs)
{
#ifdef SIM
	return;
#else
	char *s = (char *) cs;
	short c;
	
	while (1) {
		c = *(short *)(s);
		if ((char)(c & 0xff))
			putc((char)(c & 0xff));
		else
			break;
		if ((char)((c >> 8) & 0xff))
			putc((char)((c >> 8) & 0xff));
		else
			break;
		s += sizeof(short);
	}
#endif
}

static void
puthex(unsigned int h)
{
#ifdef SIM
	return;
#else
	char c;
	int i;
	
	for (i = 7; i >= 0; i--) {
		c = (char)((h >> (i * 4)) & 0xf);
		c += (c > 9) ? ('a' - 10) : '0';
		putc(c);
	}
#endif
}

void
putdec(unsigned int d)
{
#ifdef SIM
	return;
#else
	int leading_zero;
	unsigned int divisor, result, remainder;

	leading_zero = 1;
	remainder = d;

	for (divisor = 1000000000; 
	     divisor > 0; 
	     divisor /= 10) {
		result = remainder / divisor;
		remainder %= divisor;

		if (result != 0 || divisor == 1)
			leading_zero = 0;

		if (leading_zero == 0)
			putc((char)(result) + '0');
	}
#endif
}

static INLINE uint32
factor6(uint32 x)
{
	switch (x) {
	case CC_F6_2:	return 2;
	case CC_F6_3:	return 3;
	case CC_F6_4:	return 4;
	case CC_F6_5:	return 5;
	case CC_F6_6:	return 6;
	case CC_F6_7:	return 7;
	default:	return 0;
	}
}

/* calculate the speed the SB would run at given a set of clockcontrol values */
static uint32
sb_clock_rate(uint32 pll_type, uint32 n, uint32 m)
{
#if 1 /* PLL clock ??? */
#warning "Fix Me....................... misc.c sb_clock_rate"
	return 100;
#else
	uint32 n1, n2, clock, m1, m2, m3, mc;

	n1 = n & CN_N1_MASK;
	n2 = (n & CN_N2_MASK) >> CN_N2_SHIFT;

	if ((pll_type == PLL_TYPE1) || (pll_type == PLL_TYPE4)) {
		n1 = factor6(n1);
		n2 += CC_F5_BIAS;
	} else if (pll_type == PLL_TYPE2) {
		n1 += CC_T2_BIAS;
		n2 += CC_T2_BIAS;
	} else if (pll_type == PLL_TYPE3) {
		return (100000000);
	}

	clock = CC_CLOCK_BASE * n1 * n2;

	if (clock == 0)
		return 0;

	m1 = m & CC_M1_MASK;
	m2 = (m & CC_M2_MASK) >> CC_M2_SHIFT;
	m3 = (m & CC_M3_MASK) >> CC_M3_SHIFT;
	mc = (m & CC_MC_MASK) >> CC_MC_SHIFT;

	if ((pll_type == PLL_TYPE1) || (pll_type == PLL_TYPE4)) {
		m1 = factor6(m1);
		if (pll_type == PLL_TYPE1)
			m2 += CC_F5_BIAS;
		else
			m2 = factor6(m2);
		m3 = factor6(m3);

		switch (mc) {
		case CC_MC_BYPASS:	return (clock);
		case CC_MC_M1:		return (clock / m1);
		case CC_MC_M1M2:	return (clock / (m1 * m2));
		case CC_MC_M1M2M3:	return (clock / (m1 * m2 * m3));
		case CC_MC_M1M3:	return (clock / (m1 * m3));
		default:		return (0);
		}
	} else {
		m1 += CC_T2_BIAS;
		m2 += CC_T2M2_BIAS;
		m3 += CC_T2_BIAS;

		if ((mc & CC_T2MC_M1BYP) == 0)
			clock /= m1;
		if ((mc & CC_T2MC_M2BYP) == 0)
			clock /= m2;
		if ((mc & CC_T2MC_M3BYP) == 0)
			clock /= m3;

		return(clock);
	}
#endif
}

static void
uart_init(int baud)
{
	sbconfig_t *sb;
	unsigned long base, hz, ns, tmp;
	int quot;

	if (eir) {
#if 0
		/* Determine external UART register base */
		sb = (sbconfig_t *)((unsigned int) eir + SBCONFIGOFF);
		base = EXTIF_CFGIF_BASE(readl(&sb->sbadmatch1) & SBAM_BASE1_MASK);

		/* Enable programmable interface */
		writel(CF_EN, &eir->prog_config);

		/* Calculate clock cycle */
		sbclock = mipsclock = hz = sb_clock_rate(PLL_TYPE1, readl(&eir->clockcontrol_n), readl(&eir->clockcontrol_sb));
		hz = hz ? : 100000000;
		ns = 1000000000 / hz;

		/* Set programmable interface timing for external uart */
		tmp = CEIL(10, ns) << FW_W3_SHIFT;	/* W3 = 10nS */
		tmp = tmp | (CEIL(20, ns) << FW_W2_SHIFT); /* W2 = 20nS */
		tmp = tmp | (CEIL(100, ns) << FW_W1_SHIFT); /* W1 = 100nS */
		tmp = tmp | CEIL(120, ns);		/* W0 = 120nS */
		writel(tmp, &eir->prog_waitcount);	/* 0x01020a0c for a 100Mhz clock */

		uart.baud_base = 13500000 / 16;  
		uart.iomem_reg_shift = 0;
		uart.iomem_base = (u8 *) KSEG1ADDR(base);
#endif
	} else if (cc) {
		uint32	rev, cap, pll_type, tmp;

		/* Determine core revision */
		sb = (sbconfig_t *)((unsigned int) cc + SBCONFIGOFF);
		rev = readl(&sb->sbidhigh) & SBIDH_RC_MASK;
		cap = readl(&cc->capabilities);
		pll_type = cap & CAP_PLL_MASK;

		/* Determine internal UART clock source */
		if (bcm_chipid ==BCM5365_DEVICE_ID) {
#ifdef CONFIG_BCM5XXX_FPGA
                            uart.baud_base = 2000000;
#else
                            uart.baud_base = 1850000;
#endif
		}else if (pll_type ==0x0010000) {
			/* PLL clock */
			uart.baud_base = sb_clock_rate(pll_type, readl(&cc->clockcontrol_n),
						       readl(&cc->clockcontrol_m2));
			sbclock = mipsclock = hz = sb_clock_rate(pll_type, readl(&cc->clockcontrol_n),
								 readl(&cc->clockcontrol_sb));
		} else {
			uint32 div;

			sbclock = hz = sb_clock_rate(pll_type, readl(&cc->clockcontrol_n),
						     readl(&cc->clockcontrol_sb));
			mipsclock = sb_clock_rate(pll_type, readl(&cc->clockcontrol_n),
						  readl(&cc->clockcontrol_mips));
			/* Internal backplane clock */
			if (rev >= 3) {
				uart.baud_base = sbclock;
				div = uart.baud_base / 1843200;
				writel(div, &cc->uart_clkdiv);
			} else {
				uart.baud_base = 88000000;
				div = 48;
			}
			if ((rev > 0) && ((readl(&cc->corecontrol) & CC_UARTCLKO) == 0)) {
				/* If UartClkOvveride is not set then t depends on strapping
				 * as reflected by the UCLKSEL field;
				 */
				if ((cap & CAP_UCLKSEL) == CAP_UINTCLK) {
					/* Internal divided backplane clock */
					uart.baud_base /= div;
				} else {
					/* Assume external clock of 1.8432 MHz */
					uart.baud_base = 1843200;
				}
			}
		}
		ns = 1000000000 / hz;

#if 0
		/* Set timing for the flash */
		tmp = CEIL(10, ns) << FW_W3_SHIFT;	/* W3 = 10nS */
		tmp |= CEIL(10, ns) << FW_W1_SHIFT;	/* W1 = 10nS */
		tmp |= CEIL(120, ns);			/* W0 = 120nS */
		writel(tmp, &cc->parallelflashwaitcnt);
#endif

		writel(tmp, &cc->cs01memwaitcnt);

		uart.baud_base /= 16;
		uart.iomem_reg_shift = 0;
#if 1
#warning "UART base Fix copied from linux "	
#define UART_BASE  0xb8000400
		uart.iomem_base = (u8 *) UART_BASE;
#if 0
	if (rev)
		uart.iomem_base = (u8 *) &cc->uart0data + (uart.line * 256);
	else
		uart.iomem_base = (u8 *) &cc->uart0data + (uart.line * 8);
#endif
#else
		uart.iomem_base = (u8 *) &cc->uart0data; 
#endif
	}

	loops_per_jiffy = 5 * (mipsclock / 1000);

	/* Set baud and 8N1 */
	quot = uart.baud_base / baud;
	serial_out(&uart, UART_LCR, UART_LCR_DLAB);
	serial_out(&uart, UART_DLL, quot & 0xff);
	serial_out(&uart, UART_DLM, quot >> 8);
	serial_out(&uart, UART_LCR, UART_LCR_WLEN8);
}

static void
reset_usb(chipcregs_t *cc, void *usb)
{
#if defined(CONFIG_USB_OHCI) || defined(CONFIG_USBDEV)
	sbconfig_t *sb;

	sb = (sbconfig_t *)((unsigned int) usb + SBCONFIGOFF);
	if ((readl(&cc->intstatus) & 0x80000000) == 0 &&
	    (readl(&sb->sbidhigh) & SBIDH_RC_MASK) == 1) {
		/* Reset USB host core into sane state */
		writel((1 << 29) | SBTML_RESET | SBTML_CLK, &sb->sbtmstatelow);
		udelay(10);
		/* Reset USB device core into sane state */
		writel(SBTML_RESET | SBTML_CLK, &sb->sbtmstatelow);
		udelay(10);
		/* Reset backplane to 96 MHz */
		writel(0x0303, &cc->clockcontrol_n);
		writel(0x04020011, &cc->clockcontrol_sb);
		writel(0x11030011, &cc->clockcontrol_pci);
		writel(0x01050811, &cc->clockcontrol_m2);
		writel(1, &cc->watchdog);
		while (1);
	}
#endif
}

static void
error(char *x)
{
	puts("\n\n");
	puts(x);
	puts("\n\n -- System halted");

	while(1);	/* Halt */
}


/*
 * gzip declarations
 */

#define OF(args) args
#define STATIC static

#undef memset
#undef memcpy
#define memzero(s, n)	memset ((s), 0, (n))

typedef unsigned char  uch;
typedef unsigned short ush;
typedef unsigned long  ulg;

#define WSIZE 0x8000		/* Window size must be at least 32k, */
				/* and a power of two */

static uch *inbuf;		/* input buffer */
static ulg tmp;
static uch window[WSIZE];	/* Sliding window buffer */

static unsigned insize;		/* valid bytes in inbuf */
static unsigned inptr;		/* index of next byte to be processed in inbuf */
static unsigned outcnt;		/* bytes in output buffer */

/* gzip flag byte */
#define ASCII_FLAG	0x01	/* bit 0 set: file probably ascii text */
#define CONTINUATION	0x02	/* bit 1 set: continuation of multi-part gzip file */
#define EXTRA_FIELD	0x04	/* bit 2 set: extra field present */
#define ORIG_NAME	0x08	/* bit 3 set: original file name present */
#define COMMENT		0x10	/* bit 4 set: file comment present */
#define ENCRYPTED	0x20	/* bit 5 set: file is encrypted */
#define RESERVED	0xC0	/* bit 6,7:   reserved */

extern uch input_data[];
extern int input_len;
extern char text_start[], text_end[];
extern char data_start[], data_end[];
extern char bss_start[], bss_end[];

static inline uch
get_byte(void)
{
	if (sflash) {
#if 0
		uch c;
		sflash_read(cc, inptr++, 1, &c);
		return c;
#endif
	} else {
		if ((inptr % 4) == 0)
			tmp = *((ulg *) &inbuf[inptr]);
		return ((uch *) &tmp)[inptr++ % 4];
	}
}	

/* Diagnostic functions */
#ifdef DEBUG
#  define Assert(cond,msg) {if(!(cond)) error(msg);}
#  define Trace(x) fprintf x
#  define Tracev(x) {if (verbose) fprintf x ;}
#  define Tracevv(x) {if (verbose>1) fprintf x ;}
#  define Tracec(c,x) {if (verbose && (c)) fprintf x ;}
#  define Tracecv(c,x) {if (verbose>1 && (c)) fprintf x ;}
#else
#  define Assert(cond,msg)
#  define Trace(x)
#  define Tracev(x)
#  define Tracevv(x)
#  define Tracec(c,x)
#  define Tracecv(c,x)
#endif

static void flush_window(void);
static void error(char *m);
static void gzip_mark(void **);
static void gzip_release(void **);

static uch *output_data;
static ulg output_ptr;
static ulg bytes_out;

static void *malloc(int size);
static void free(void *where);
static void error(char *m);
static void gzip_mark(void **);
static void gzip_release(void **);

static void puts(const char *);

extern int end;
static ulg free_mem_ptr;
static ulg free_mem_ptr_end;

#define HEAP_SIZE 0x2000

#include "../../../../../lib/inflate.c"

static void *
malloc(int size)
{
	void *p;

	if (size <0) error("Malloc error\n");
	if (free_mem_ptr <= 0) error("Memory error\n");

	free_mem_ptr = (free_mem_ptr + 3) & ~3;	/* Align */

	p = (void *)free_mem_ptr;
	free_mem_ptr += size;

	if (free_mem_ptr >= free_mem_ptr_end)
		error("Out of memory");
	return p;
}

static void
free(void *where)
{ /* gzip_mark & gzip_release do the free */
}

static void
gzip_mark(void **ptr)
{
	*ptr = (void *) free_mem_ptr;
}

static void
gzip_release(void **ptr)
{
	free_mem_ptr = (long) *ptr;
}

void*
memset(void* s, int c, size_t n)
{
	int i;
	char *ss = (char*)s;

	for (i=0;i<n;i++) ss[i] = c;
	return s;
}

void*
memcpy(void* __dest, __const void* __src, size_t __n)
{
	int i;
	char *d = (char *)__dest, *s = (char *)__src;

	for (i=0;i<__n;i++) d[i] = s[i];
	return __dest;
}

/* ===========================================================================
 * Write the output window window[0..outcnt-1] and update crc and bytes_out.
 * (Used for the decompressed data only.)
 */
void
flush_window(void)
{
	ulg c = crc;
	unsigned n;
	uch *in, *out, ch;

	in = window;
	out = &output_data[output_ptr];
	for (n = 0; n < outcnt; n++) {
		ch = *out++ = *in++;
		c = crc_32_tab[((int)c ^ ch) & 0xff] ^ (c >> 8);
	}
	crc = c;
	bytes_out += (ulg)outcnt;
	output_ptr += (ulg)outcnt;
	outcnt = 0;
	puts(".");
}

static void
decompress_kernel(void)
{
#if 1  /* Fix Me */
	inbuf = (uch *) 0x80500000;
#else
	/* Decompress from flash */
	inbuf = (uch *) KSEG1ADDR(0x1fc00000);
#endif
	insize = input_len;
	inptr = (unsigned) input_data - (unsigned) text_start;
	output_data = (uch *) LOADADDR;
	free_mem_ptr = (ulg) bss_end;
	free_mem_ptr_end = (ulg) bss_end + 0x100000;

	makecrc();
	puts("Uncompressing Linux...");

	gunzip();

	puts("done, booting the kernel.\n");

#if 0
	/* Flush all caches */
	blast_dcache();
	blast_icache();
#endif

	/* Jump to kernel */
	((void (*)(void)) LOADADDR)();
}

#if 0
static void
sflash_self(chipcregs_t *cc)
{
	unsigned char *start = text_start;
	unsigned char *end = data_end;
	unsigned char *cur = start;
	unsigned int erasesize, len;

	while (cur < end) {
		/* Erase sector */
		puts("Erasing sector 0x");
		puthex(cur - start);
		puts("...");
		if ((erasesize = sflash_erase(cc, cur - start)) < 0) {
			puts("error\n");
			break;
		}
		while (sflash_poll(cc, cur - start));
		puts("done\n");

		/* Write sector */
		puts("Writing sector 0x");
		puthex(cur - start);
		puts("...");
		while (erasesize) {
			if ((len = sflash_write(cc, cur - start, erasesize, cur)) < 0)
				break;
			while (sflash_poll(cc, cur - start));
			cur += len;
			erasesize -= len;
		}
		if (erasesize) {
			puts("error\n");
			break;
		}
		puts("done\n");
	}
}

static void
_change_cachability(u32 cm)
{
	u32 prid;

	change_c0_config(CONF_CM_CMASK, cm);
	prid = read_c0_prid();
	if ((prid & (PRID_COMP_MASK | PRID_IMP_MASK)) ==
	    (PRID_COMP_BROADCOM | PRID_IMP_BCM3302)) {
		cm = read_c0_diag();
		/* Enable icache */
		cm |= (1 << 31);
		/* Enable dcache */
		cm |= (1 << 30);
		write_c0_diag(cm);
	}
}	
static void (*change_cachability)(u32);

#define HANDLER_ADDR 0xa0000180

void
handler(void)
{
	/* enable interrupts */
	clear_c0_status(IE_IRQ5 | IE_IRQ4 | IE_IRQ3 | IE_IRQ2 | IE_IRQ1 | IE_IRQ0 | ST0_IE);

	__asm__ __volatile__ (".set\tmips32\n\t"
		"ssnop\n\t"
		"ssnop\n\t"
		"eret\n\t"
		"nop\n\t"
		"nop\n\t"
		".set\tmips0");			/* step 11 */
}
/* The followint MUST come right after handler() */
void
afterhandler(void)
{
}

#define	BCM4704_DEFAULT_MIPS_CLOCK	200000000
static unsigned int target_mips_clock = 264000000;
static unsigned int target_sb_clock   = 132000000;

typedef struct {
	uint32	mipsclock;
	uint32	sbclock;
	uint16	n;
	uint32	sb;
	uint32	pci;
	uint32	m2;
	uint32	m3;
	uint	ratio;
	uint32	ratio_parm;
} sb_clock_table_t;

static sb_clock_table_t sb_clock_table[] = {
	{ 180000000,  90000000, 0x0403, 0x02000002, 0x00000002, 0x02000002, 0x06000002, 0x21, 0x0aaa0555},
	{ 200000000, 100000000, 0x0303, 0x02010000, 0x02040001, 0x02010000, 0x06000001, 0x21, 0x0aaa0555},
	{ 264000000, 132000000, 0x0903, 0x02000003, 0x04000702, 0x02000003, 0x06000003, 0x21, 0x0aaa0555},
	{ 280000000, 140000000, 0x0503, 0x02010000, 0x00010001, 0x02010000, 0x06000001, 0x21, 0x0aaa0555},
	{ 288000000, 144000000, 0x0404, 0x02010000, 0x00010001, 0x02010000, 0x06000001, 0x21, 0x0aaa0555},
	{ 300000000, 150000000, 0x0803, 0x02000002, 0x00010002, 0x02000002, 0x06000002, 0x21, 0x0aaa0555},
	{ 180000000,  80000000, 0x0403, 0x02010001, 0x00000002, 0x00010101, 0x06000002, 0x49, 0x012A00A9},
	{ 234000000, 104000000, 0x0b01, 0x02010001, 0x04000204, 0x00010101, 0x06000002, 0x49, 0x01250125},
	{ 300000000, 133333333, 0x0803, 0x02010001, 0x00010101, 0x00010101, 0x06000002, 0x49, 0x012a0115},
	{ 0 }
};

void
change_clock(void)
{
	int c;
	u32 s, e, d, i, tmp, ratio_parm;
	sb_clock_table_t *cte, *ccte = NULL, *tcte = NULL;

	/* Change the clock and reboot if needed */
	/* Gross hack for now to go all the way */
	if (chipid == BCM4704_DEVICE_ID) {
		if (mipsclock != BCM4704_DEFAULT_MIPS_CLOCK) {
			target_mips_clock = mipsclock;
			target_sb_clock = sbclock;
		}
		if ((mipsclock != target_mips_clock) || (sbclock != target_sb_clock)) {
			for (cte = sb_clock_table; cte->mipsclock; cte++) {
				if ((cte->mipsclock == mipsclock) && (cte->sbclock == sbclock))
					ccte = cte;
				if ((cte->mipsclock == target_mips_clock) && (cte->sbclock == target_sb_clock))
					tcte = cte;
			}

			if ((ccte == NULL) || (tcte == NULL)) {
				puts("\nCould not figure out current or target settings");
				goto nochange;
			}

			puts("Run at ");
			putdec(tcte->mipsclock);
			putc('/');
			putdec(tcte->sbclock);
			putc('?');
			c = 'y';
			for (i = 50; i; i--) {
				if (keyhit()) {
					c = getc() | 0x20;
					break;
				}
				if ((i % 10) == 0)
					putc('.');
				mdelay(100);
			}
			if (c != 'y') {
				for (i = 0, cte = sb_clock_table; cte->mipsclock; i++, cte++) {
					puts("\n    [");
					putdec(i);
					puts("] = ");
					putdec(cte->mipsclock);
					putc('/');
					putdec(cte->sbclock);
					if (cte == tcte)
						putc('*');
				}

				while (1) {
					puts("\nChange to ?");
					c = getc() - '0';
					if ((c >= 0) && (c < i))
						tcte = &sb_clock_table[c];
					else {
						puts("\nPlase type a number from 0 to ");
						putdec(i - 1);
						continue;
					}
					target_mips_clock = tcte->mipsclock;
					target_sb_clock = tcte->sbclock;
					puts("\nChanging to ");
					putdec(tcte->mipsclock);
					putc('/');
					putdec(tcte->sbclock);
					puts(", ok?");
					c = getc() | 0x20;
					if (c == 'y')
						break;
				}
			}
			if (tcte == ccte)
				goto nochange;

			/* Set the pll controls now */
			writel(tcte->n, &cc->clockcontrol_n);
			writel(tcte->sb, &cc->clockcontrol_sb);
			writel(tcte->pci, &cc->clockcontrol_pci);
			writel(tcte->m2, &cc->clockcontrol_m2);
			writel(tcte->m3, &cc->clockcontrol_mips);

			if (tcte->ratio_parm != ccte->ratio_parm) {
				puts("\nChanging ratio_parm to 0x");
				puthex(tcte->ratio_parm);
				puts(", type new one to override: ");
				ratio_parm = 0;
				while (1) {
					c = getc() & 0x7f;
					putc(c);
					if ((c == 'x') || (c == 'X')) {
						ratio_parm = 0;
						continue;
					}
					if ((c >= '0') && (c <= '9')) {
						ratio_parm = (ratio_parm << 4) + (c - '0');
						continue;
					}
					c &= ~0x20;
					if ((c >= 'A') && (c <= 'F'))
						ratio_parm = (ratio_parm << 4) + (c - 'A' + 10);
					else
						break;
				}
				putc('\n');

				if (ratio_parm == 0)
					ratio_parm = tcte->ratio_parm;

				/* Preload the code in the cache */
				s = ((u32)&&start_fill) & ~(ic_lsize - 1);
				e = (((u32)&&end_fill) + (ic_lsize - 1)) & ~(ic_lsize - 1);
				while (s < e) {
					cache_unroll(s, Fill);
					s += ic_lsize;
				}

				/* Copy the handler & preload it into the cache */
				s = (u32)&handler;
				e = (u32)&afterhandler;
				d = HANDLER_ADDR;
				while (s < e) {
					for (i = 0; i < ic_lsize; i += 4) {
						*(long*)(d + i) = *(long *)(s + i);
					}
					cache_unroll(d, Fill);
					s += ic_lsize;
					d += ic_lsize;
				}

				/* Clear BEV bit */
				clear_c0_status(ST0_BEV);

				/* enable interrupts */
				set_c0_status(IE_IRQ4 | IE_IRQ3 | IE_IRQ2 | IE_IRQ1 | IE_IRQ0 | ST0_IE);
				/* enable timer interrupts */
				writel(1, &mipsr->intmask);

start_fill:
				/* step 1, set clock ratios */
				write_c0_diag3(ratio_parm);
				write_c0_diag1(8);

				/* step 2: program timer intr */
				writel(100, &mipsr->timer);
				tmp = readl(&mipsr->timer);	/* read it back to sync */

				/* step 3, switch to async */
				write_c0_diag4(1 << 22);

				/* step 4, set cfg active */
				write_c0_diag2(0x9);

				/* steps 5 & 6 */ 
				__asm__ __volatile__(".set\tmips3\n\t"
						     "wait\n\t"
						     ".set\tmips0");

				/* step 7, clear cfg_active */
				write_c0_diag2(0);

				/* step 8, fake soft reset */
				write_c0_diag5(read_c0_diag5() | 4);
			}

			/* step 9 set watchdog timer */
			writel(20, &cc->watchdog);
			tmp = readl(&cc->chipid);	/* dummy read */

			/* step 11 */
			__asm__ __volatile__(".set\tmips3\n\t"
					     "sync\n\t"
					     "wait\n\t"
					     ".set\tmips0");
			while (1);
		} else {
end_fill:
nochange:
			puts("\nNot changing clock. mips=");
			putdec(target_mips_clock);
			putc('/');
			putdec(target_sb_clock);
			putc('\n');
		}
	} else {
		puts("Not a 4704, not changing clock\n");
	}
}
#endif

void
c_main(unsigned long ra)
{
	/* Disable interrupts */
	clear_c0_status(1);

	/* Scan backplane */
	sb_scan();

	if (cc && usb && PHYSADDR(ra) >= 0x1fc00000)
		reset_usb(cc, usb);


	/* Determine chip ID and revision */
	if (cc) {
		bcm_chipid = readl(&cc->chipid) & CID_ID_MASK;
		bcm_chiprev = (readl(&cc->chipid) & CID_REV_MASK) >> CID_REV_SHIFT;
	}

	/* Initialize UART */
	uart_init(115200);
#if 0
	puts("\nSelf-booting Linux running on a ");
	puthex(chipid);
	puts(" Rev. ");
	putdec(chiprev);
	puts(" @ ");
	putdec(mipsclock);
	putc('/');
	putdec(sbclock);
	putc('\n');

	puts("CP0 PRID: 0x");
	puthex(read_c0_prid());
	putc('\n');
	puts("CP0 Conf: 0x");
	puthex(read_c0_conf());
	putc('\n');
	puts("CP0 Info: 0x");
	puthex(read_c0_info());
	putc('\n');
	puts("CP0 Status: 0x");
	puthex(read_c0_status());
	putc('\n');
	puts("CP0 Cause: 0x");
	puthex(read_c0_cause());
	putc('\n');
	puts("CP0 Config: 0x");
	puthex(read_c0_config());
	putc('\n');
	puts("CP0 Config1: 0x");
	puthex(read_c0_config1());
	putc('\n');
	if (mipscore == SB_MIPS33)
		puts("CP0 Reg22: sel0/1/2/3/4/5:\n    ");
	else
		puts("CP0 Reg22: 0x");
	puthex(read_c0_diag());
	if (mipscore == SB_MIPS33) {
		puts("\n    ");
		puthex(read_c0_diag1());
		puts("\n    ");
		puthex(read_c0_diag2());
		puts("\n    ");
		puthex(read_c0_diag3());
		puts("\n    ");
		puthex(read_c0_diag4());
		puts("\n    ");
		puthex(read_c0_diag5());
	}
	putc('\n');

	if (memc) {
		puts("memc config: 0x");
		puthex(readl(&memc->config));
		putc('\n');
		puts("memc mode: 0x");
		puthex(readl(&memc->modebuf));
		putc('\n');
		puts("memc wrncdl: 0x");
		puthex(readl(&memc->wrncdlcor));
		putc('\n');
		puts("memc rdncdl: 0x");
		puthex(readl(&memc->rdncdlcor));
		putc('\n');
		puts("memc miscdly: 0x");
		puthex(readl(&memc->miscdlyctl));
		putc('\n');
		puts("memc dqsgate: 0x");
		puthex(readl(&memc->dqsgatencdl));
		putc('\n');
	}
#endif
#if 0
	/* Switch back to sync */
	write_c0_diag4(0);
#endif
#if 1
#warning "Fix cache init "
#else
	/* Must be in KSEG1 to change cachability */
	cache_init();
	change_cachability = (void (*)(u32)) KSEG1ADDR((unsigned long)(_change_cachability));
	change_cachability(CONF_CM_CACHABLE_NONCOHERENT);

	/* Change clock if needed */
	change_clock();

	/* Initialize serial flash */
	sflash = cc ? sflash_init(cc) : NULL;

	if (sflash) 
		puts("sflash set???");

	/* Copy self to flash if we booted from SDRAM */
	if (PHYSADDR(ra) < 0x1fc00000) {
		if (sflash)
			sflash_self(cc);
	}
#endif

	/* Decompress kernel */
	decompress_kernel();
}
