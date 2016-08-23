/*
 * BCM47XX FLASH driver interface
 *
 * Copyright 2001-2003, Broadcom Corporation   
 * All Rights Reserved.   
 *    
 * THIS SOFTWARE IS OFFERED "AS IS", AND BROADCOM GRANTS NO WARRANTIES OF ANY   
 * KIND, EXPRESS OR IMPLIED, BY STATUTE, COMMUNICATION OR OTHERWISE. BROADCOM   
 * SPECIFICALLY DISCLAIMS ANY IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS   
 * FOR A SPECIFIC PURPOSE OR NONINFRINGEMENT CONCERNING THIS SOFTWARE.   
 * $Id$
 */

#ifndef _flashutl_h_
#define _flashutl_h_

#define FLASH_BASE      0xbfc00000		/* BCM4710 */

int	flash_init(void* base_addr, char *flash_str);
int	flash_erase(void);
int	flash_eraseblk(unsigned long off);
int	flash_write(unsigned long off, uint16 *src, uint nbytes);
unsigned long	flash_block_base(unsigned long off);
unsigned long	flash_block_lim(unsigned long off);
int FlashWriteRange(unsigned short* dst, unsigned short* src, unsigned int numbytes);

void nvWrite(unsigned short *data, unsigned int len);

/* Global vars */
extern char*		flashutl_base;
extern flash_desc_t*	flashutl_desc;
extern flash_cmds_t*	flashutl_cmd;

#endif /* _flashutl_h_ */
