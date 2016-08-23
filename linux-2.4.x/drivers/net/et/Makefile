#  Copyright 2001-2003, Broadcom Corporation   
#  All Rights Reserved.   
#     
#  THIS SOFTWARE IS OFFERED "AS IS", AND BROADCOM GRANTS NO WARRANTIES OF ANY   
#  KIND, EXPRESS OR IMPLIED, BY STATUTE, COMMUNICATION OR OTHERWISE. BROADCOM   
#  SPECIFICALLY DISCLAIMS ANY IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS   
#  FOR A SPECIFIC PURPOSE OR NONINFRINGEMENT CONCERNING THIS SOFTWARE.   
#
# Makefile for the Broadcom et driver
#
# $Id$
#

O_TARGET	:= et.o

ET_OBJS		:= et_linux.o etc.o et_test.o

ifeq ($(CONFIG_ET_47XX),y)
ET_OBJS		+= etc47xx.o etc_robo.o etc_adm.o
EXTRA_CFLAGS	+= -DBCM47XX_CHOPS
endif
ifeq ($(CONFIG_ET_4413),y)
ET_OBJS		+= etc4413.o
EXTRA_CFLAGS	+= -DBCM4413_CHOPS
endif

export-objs	:=
obj-y		:= $(ET_OBJS)
obj-m		:= $(O_TARGET)

SRCBASE		:= $(TOPDIR)/../..
EXTRA_CFLAGS	+= -I$(SRCBASE)/include
EXTRA_CFLAGS	+= -I$(SRCBASE)/switch/sys
EXTRA_CFLAGS	+= -DDMA

# Search for sources under src/et/sys or objects under src/et/linux
ifneq ($(wildcard $(SRCBASE)/et/sys),)
EXTRA_CFLAGS	+= -I$(SRCBASE)/et/sys
vpath %.c $(SRCBASE)/et/sys $(SRCBASE)/shared
else
obj-y		:= $(foreach obj,$(ET_OBJS),$(SRCBASE)/et/linux/$(obj))
endif

include $(TOPDIR)/Rules.make
