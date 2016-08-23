#ifndef _S5_H_
#define _S5_H_
/*
 *   Copyright 2003, Broadcom Corporation
 *   All Rights Reserved.
 * 
 *   Broadcom Sentry5 (S5) BCM5365, 53xx, BCM58xx SOC Internal Core
 *   and MIPS3301 (R4K) System Address Space
 *
 *   This program is free software; you can redistribute it and/or
 *   modify it under the terms of the GNU General Public License as
 *   published by the Free Software Foundation, located in the file
 *   LICENSE.
 *
 *   $Id: s5.h,v 1.3 2003/06/10 18:54:51 jfd Exp $
 * 
 */

/* BCM5365 Address map */
#define KSEG1ADDR(x)    ( (x) | 0xa0000000)
#define BCM5365_SDRAM		0x00000000 /* 0-128MB Physical SDRAM */
#define BCM5365_PCI_MEM		0x08000000 /* Host Mode PCI mem space (64MB) */
#define BCM5365_PCI_CFG		0x0c000000 /* Host Mode PCI cfg space (64MB) */
#define BCM5365_PCI_DMA		0x40000000 /* Client Mode PCI mem space (1GB)*/
#define	BCM5365_SDRAM_SWAPPED	0x10000000 /* Byteswapped Physical SDRAM */
#define BCM5365_ENUM		0x18000000 /* Beginning of core enum space */

/* BCM5365 Core register space */
#define BCM5365_REG_CHIPC	0x18000000 /* Chipcommon  registers */
#define BCM5365_REG_EMAC0	0x18001000 /* Ethernet MAC0 core registers */
#define BCM5365_REG_IPSEC	0x18002000 /* BCM582x CryptoCore registers */
#define BCM5365_REG_USB		0x18003000 /* USB core registers */
#define BCM5365_REG_PCI		0x18004000 /* PCI core registers */
#define BCM5365_REG_MIPS33	0x18005000 /* MIPS core registers */
#define BCM5365_REG_MEMC	0x18006000 /* MEMC core registers */
#define BCM5365_REG_UARTS       (BCM5365_REG_CHIPC + 0x300) /* UART regs */
#define	BCM5365_EJTAG		0xff200000 /* MIPS EJTAG space (2M) */

/* COM Ports 1/2 */
#define	BCM5365_UART		(BCM5365_REG_UARTS)
#define BCM5365_UART_COM2	(BCM5365_REG_UARTS + 0x00000100)

/* Registers common to MIPS33 Core used in 5365 */
#define MIPS33_FLASH_REGION           0x1fc00000 /* Boot FLASH Region  */
#define MIPS33_EXTIF_REGION           0x1a000000 /* Chipcommon EXTIF region*/
#define BCM5365_EXTIF                 0x1b000000 /* MISC_CS */
#define MIPS33_FLASH_REGION_AUX       0x1c000000 /* FLASH Region 2*/

/* Internal Core Sonics Backplane Devices */
#define INTERNAL_UART_COM1            BCM5365_UART
#define INTERNAL_UART_COM2            BCM5365_UART_COM2
#define SB_REG_CHIPC                  BCM5365_REG_CHIPC
#define SB_REG_ENET0                  BCM5365_REG_EMAC0
#define SB_REG_IPSEC                  BCM5365_REG_IPSEC
#define SB_REG_USB                    BCM5365_REG_USB
#define SB_REG_PCI                    BCM5365_REG_PCI
#define SB_REG_MIPS                   BCM5365_REG_MIPS33
#define SB_REG_MEMC                   BCM5365_REG_MEMC
#define SB_REG_MEMC_OFF               0x6000
#define SB_EXTIF_SPACE                MIPS33_EXTIF_REGION
#define SB_FLASH_SPACE                MIPS33_FLASH_REGION

/*
 * XXX
 * 5365-specific backplane interrupt flag numbers.  This should be done
 * dynamically instead.
 */
#define	SBFLAG_PCI	0
#define	SBFLAG_ENET0	1
#define	SBFLAG_ILINE20	2
#define	SBFLAG_CODEC	3
#define	SBFLAG_USB	4
#define	SBFLAG_EXTIF	5
#define	SBFLAG_ENET1	6

/* BCM95365 Local Bus devices */
#define BCM95365K_RESET_ADDR    	 BCM5365_EXTIF
#define BCM95365K_BOARDID_ADDR  	(BCM5365_EXTIF | 0x4000)
#define BCM95365K_DOC_ADDR      	(BCM5365_EXTIF | 0x6000)
#define BCM95365K_LED_ADDR      	(BCM5365_EXTIF | 0xc000)
#define BCM95365K_TOD_REG_BASE          (BCM95365K_NVRAM_ADDR | 0x1ff0)
#define BCM95365K_NVRAM_ADDR    	(BCM5365_EXTIF | 0xe000)
#define BCM95365K_NVRAM_SIZE             0x1ff0 /* 8K NVRAM : DS1743/STM48txx*/

/* Write to DLR2416 VFD Display character RAM */
#define LED_REG(x)      \
 (*(volatile unsigned char *) (KSEG1ADDR(BCM95365K_LED_ADDR) + (x)))

#ifdef	CONFIG_VSIM
#define	BCM5365_TRACE(trval)        do { *((int *)0xa0002ff8) = (trval); \
                                       } while (0)
#else
#define	BCM5365_TRACE(trval)        do { *((unsigned char *)\
                                         KSEG1ADDR(BCM5365K_LED_ADDR)) = (trval); \
				    *((int *)0xa0002ff8) = (trval); } while (0)
#endif

/* BCM9536R Local Bus devices */
#define BCM95365R_DOC_ADDR      	BCM5365_EXTIF



#endif /*!_S5_H_ */
