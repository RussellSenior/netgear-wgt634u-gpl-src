/*
 * Broadcom SiliconBackplane chipcommon serial flash interface
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

#ifndef _sflash_h_
#define _sflash_h_

#include <typedefs.h>
#include <sbchipc.h>

/* GPIO based bank selection (1 GPIO bit) */
#define SFLASH_MAX_BANKS	1
#define SFLASH_GPIO_SHIFT	2
#define SFLASH_GPIO_MASK	((SFLASH_MAX_BANKS - 1) << SFLASH_GPIO_SHIFT)

struct sflash_bank {
	uint offset;					/* Byte offset */
	uint erasesize;					/* Block size */
	uint numblocks;					/* Number of blocks */
	uint size;					/* Total bank size in bytes */
};

struct sflash {
	struct sflash_bank banks[SFLASH_MAX_BANKS];	/* GPIO selectable banks */
	uint32 type;					/* Type */
	uint size;					/* Total array size in bytes */
};

/* Utility functions */
extern int sflash_poll(chipcregs_t *cc, uint offset);
extern int sflash_read(chipcregs_t *cc, uint offset, uint len, uchar *buf);
extern int sflash_write(chipcregs_t *cc, uint offset, uint len, const uchar *buf);
extern int sflash_erase(chipcregs_t *cc, uint offset);
extern struct sflash * sflash_init(chipcregs_t *cc);

#endif /* _sflash_h_ */
