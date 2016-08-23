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

# Link at 3 MB offset in RAM
TEXT_START     := 0x80500000                                                   
ifdef TEXTADDR                                                                 
LOADADDR       := $(TEXTADDR)                                                  
endif                                                                          
                                                                               
STRIP := $(CROSS_COMPILE)strip

OBJCOPY		:= $(CROSS_COMPILE)objcopy -O binary -R .reginfo -R .note -R .comment -R .mdebug -S

SRCBASE		:= $(TOPDIR)/../..
VPATH		:= $(SRCBASE)/shared
### Fix it by getting it from the Master rules
ASFLAGS		+= -G 0  -mno-abicalls -fno-pic -pipe -mcpu=r4600 -mips2 -Wa --trap -m4710a0kern
ASFLAGS		+= -D__ASSEMBLY__ -I$(SRCBASE)/include -DLOADADDR=$(LOADADDR)
CFLAGS		+= -I$(SRCBASE)/include -DLOADADDR=$(LOADADDR)
ifdef CONFIG_MCOUNT
CFLAGS		:= $(subst -pg,,$(CFLAGS))
endif
SEDFLAGS	:= s/TEXT_START/$(TEXT_START)/

SYSTEM         := $(TOPDIR)/vmlinux                                            
#OBJECTS               := head.o sbsdram.o misc.o sflash.o                     
# Don't use nvram or dram initalization.Hope cfe to do it or kernel.           
OBJECTS                := head.o  misc.o

all: zImage

# Don't build dependencies, this may die if $(CC) isn't gcc
dep:

zImage: vmlinux
	$(OBJCOPY) $< $@

vmlinux: vmlinux.lds $(OBJECTS) piggy.o
	$(LD) -no-warn-mismatch -T vmlinux.lds -o $@ $(OBJECTS) piggy.o         
	$(STRIP) $@

vmlinux.lds: vmlinux.lds.in Makefile
	@sed "$(SEDFLAGS)" < $< > $@

piggy.o: $(SYSTEM)
	$(STRIP) $(SYSTEM)                                                      
	$(OBJCOPY) $(SYSTEM) piggy                                              
	gzip -c9 piggy > vmlinuz                                                
	echo "SECTIONS { .data : { input_len = .; LONG(input_data_end - input_data) input_data = .; *(.data) input_data_end = .; }}" > piggy.lnk               
	$(LD) -no-warn-mismatch -T piggy.lnk -r -o $@ -b binary vmlinuz -b elf32-tradlittlemips

mrproper: clean

clean:
	rm -f vmlinux vmlinuz zImage vmlinux.lds piggy piggy.lnk *.o
