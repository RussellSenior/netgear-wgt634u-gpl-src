#  Copyright 2001-2003, Broadcom Corporation   
#  All Rights Reserved.   
#     
#  THIS SOFTWARE IS OFFERED "AS IS", AND BROADCOM GRANTS NO WARRANTIES OF ANY   
#  KIND, EXPRESS OR IMPLIED, BY STATUTE, COMMUNICATION OR OTHERWISE. BROADCOM   
#  SPECIFICALLY DISCLAIMS ANY IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS   
#  FOR A SPECIFIC PURPOSE OR NONINFRINGEMENT CONCERNING THIS SOFTWARE.   
#
# Makefile for Broadcom Home Networking Division (HND) shared driver code
#
# $Id$
#

O_TARGET	:= hnd.o

HND_OBJS	:= bcmutils.o hnddma.o linux_osl.o sbutils.o bcmsrom.o

ifneq ($(CONFIG_BCM947XX),y)
HND_OBJS	+= nvramstubs.o
endif

export-objs	:= shared_ksyms.o
obj-y		:= shared_ksyms.o $(HND_OBJS)
obj-m		:= $(O_TARGET)

SRCBASE		:= $(TOPDIR)/../..
EXTRA_CFLAGS	+= -I$(SRCBASE)/include

vpath %.c $(SRCBASE)/shared $(SRCBASE)/shared/nvram $(SRCBASE)/shared/none

include $(TOPDIR)/Rules.make

ifeq ($(wildcard $(SRCBASE)/shared/bcmutils.c),)
bcmutils.o: $(SRCBASE)/shared/linux/bcmutils.o
	cp $< $@
endif

ifeq ($(wildcard $(SRCBASE)/shared/hnddma.c),)
hnddma.o: $(SRCBASE)/shared/linux/hnddma.o
	cp $< $@
endif

ifeq ($(wildcard $(SRCBASE)/shared/linux_osl.c),)
linux_osl.o: $(SRCBASE)/shared/linux/linux_osl.o
	cp $< $@
endif

ifeq ($(wildcard $(SRCBASE)/shared/sbutils.c),)
sbutils.o: $(SRCBASE)/shared/linux/sbutils.o
	cp $< $@
endif

ifeq ($(wildcard $(SRCBASE)/shared/bcmsrom.c),)
bcmsrom.o: $(SRCBASE)/shared/linux/bcmsrom.o
	cp $< $@
endif

ifeq ($(wildcard $(SRCBASE)/shared/nvramstubs.c),)
nvramstubs.o: $(SRCBASE)/shared/linux/nvramstubs.o
	cp $< $@
endif

shared_ksyms.c: shared_ksyms.sh $(HND_OBJS)
	sh -e $< $(HND_OBJS) > $@
