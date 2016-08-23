#
# Makefile for Broadcom BCM947XX boards
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

O_TARGET	:= bcm947xx.o

export-objs	:= cfe_flash_nvram.o setup.o
obj-y		:= prom.o setup.o time.o sbmips.o sbpci.o pcibios.o perfcntr.o gpio.o
obj-y		+= sflash.o cfe_flash_nvram.o

SRCBASE		:= $(TOPDIR)/../..

vpath %.c $(SRCBASE)/shared $(SRCBASE)/shared/nvram

include $(TOPDIR)/Rules.make
