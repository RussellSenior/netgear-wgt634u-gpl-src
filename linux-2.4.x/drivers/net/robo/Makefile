#  Copyright 2001-2003, Broadcom Corporation   
#  All Rights Reserved.   
#     
#  THIS SOFTWARE IS OFFERED "AS IS", AND BROADCOM GRANTS NO WARRANTIES OF ANY   
#  KIND, EXPRESS OR IMPLIED, BY STATUTE, COMMUNICATION OR OTHERWISE. BROADCOM   
#  SPECIFICALLY DISCLAIMS ANY IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS   
#  FOR A SPECIFIC PURPOSE OR NONINFRINGEMENT CONCERNING THIS SOFTWARE.   
#
# Makefile for the Broadcom Robo driver
#
# $Id$
#

O_TARGET	:= robo.o

ROBO_OBJS	:= swmod.o swapi.o if_robo.o osl_linux.o igmp.o brcmtag.o

export-objs	:=
obj-y		:= $(ROBO_OBJS)
obj-m		:= $(O_TARGET)

SRCBASE		:= $(TOPDIR)/../..
EXTRA_CFLAGS	+= -I$(SRCBASE)/include -I$(SRCBASE)/switch/sys

# Search for sources under src/switch/sys or objects under src/robo/linux
ifneq ($(wildcard $(SRCBASE)/switch/swmod/swmod.c),)
#EXTRA_CFLAGS	+= -I$(SRCBASE)/switch/swmod -DPORT_SCAN_TIME=10000
EXTRA_CFLAGS	+= -I$(SRCBASE)/switch/swmod -DPORT_SCAN_TIME=0
EXTRA_CFLAGS	+= -I$(SRCBASE)/et/sys
vpath %.c $(SRCBASE)/switch/sys $(SRCBASE)/switch/swmod
else
obj-y		:= $(foreach obj,$(ROBO_OBJS),$(SRCBASE)/robo/linux/$(obj))
endif

include $(TOPDIR)/Rules.make
