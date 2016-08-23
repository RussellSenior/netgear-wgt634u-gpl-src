#
# Makefile for the Broadcom wl driver
#
# Copyright 2001-2003, Broadcom Corporation
# All Rights Reserved.
# 
# THIS SOFTWARE IS OFFERED "AS IS", AND BROADCOM GRANTS NO WARRANTIES OF ANY
# KIND, EXPRESS OR IMPLIED, BY STATUTE, COMMUNICATION OR OTHERWISE. BROADCOM
# SPECIFICALLY DISCLAIMS ANY IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS
# FOR A SPECIFIC PURPOSE OR NONINFRINGEMENT CONCERNING THIS SOFTWARE.
#
# $Id$
#

O_TARGET	:= wl.o

WL_OBJS		:= wl_linux.o wlc.o d11ucode.o wlc_phy.o wlc_rate.o wlc_led.o wlc_cmn_ioctl.o wlc_security.o rc4.o tkhash.o

# Prefix driver variants
WL_APOBJS	:= $(foreach obj,$(WL_OBJS),ap_$(obj))
WL_STAOBJS	:= $(foreach obj,$(WL_OBJS),sta_$(obj))
WL_APSTAOBJS	:= $(foreach obj,$(WL_OBJS),apsta_$(obj))

# Either or both
ifeq ($(CONFIG_WL_AP),y)
AP		:= AP
endif
ifeq ($(CONFIG_WL_STA),y)
STA		:= STA
endif

# Build all variants as modules but link only one of them
export-objs	:=
obj-y		:= $(WL_$(AP)$(STA)OBJS)
obj-m		:= $(O_TARGET)
variant-objs	:= $(WL_APOBJS) $(WL_STAOBJS) $(WL_APSTAOBJS)

SRCBASE		:= $(TOPDIR)/../..
EXTRA_CFLAGS	+= -I$(SRCBASE)/include
EXTRA_CFLAGS	+= -DDMA

# Search for sources under src/wl/sys or objects under src/wl/linux
ifneq ($(wildcard $(SRCBASE)/wl/sys),)
EXTRA_CFLAGS	+= -I$(SRCBASE)/wl/sys
vpath %.c $(SRCBASE)/wl/sys $(SRCBASE)/shared $(SRCBASE)/crypto
else
obj-y		:= $(foreach obj,$(obj-y),$(SRCBASE)/wl/linux/$(obj))
variant-objs	:= $(foreach obj,$(variant-objs),$(SRCBASE)/wl/linux/$(obj))
endif

include $(TOPDIR)/Rules.make

