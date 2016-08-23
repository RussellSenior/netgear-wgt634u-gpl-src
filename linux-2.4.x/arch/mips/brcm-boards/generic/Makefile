#  Copyright 2001-2003, Broadcom Corporation   
#  All Rights Reserved.   
#     
#  THIS SOFTWARE IS OFFERED "AS IS", AND BROADCOM GRANTS NO WARRANTIES OF ANY   
#  KIND, EXPRESS OR IMPLIED, BY STATUTE, COMMUNICATION OR OTHERWISE. BROADCOM   
#  SPECIFICALLY DISCLAIMS ANY IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS   
#  FOR A SPECIFIC PURPOSE OR NONINFRINGEMENT CONCERNING THIS SOFTWARE.   
#
# Makefile for generic Broadcom MIPS boards
#
# $Id$
#

.S.s:
	$(CPP) $(AFLAGS) $< -o $*.s
.S.o:
	$(CC) $(AFLAGS) -c $< -o $*.o

O_TARGET	:= brcm.o

obj-y		:= int-handler.o irq.o

obj-$(CONFIG_REMOTE_DEBUG)	+= gdb_hook.o

SRCBASE		:= $(TOPDIR)/../..

include $(TOPDIR)/Rules.make
