###########################################################################
##
## Copyright (c) 2000 Intel Corporation 
## All rights reserved. 
##
## Redistribution and use in source and binary forms, with or without 
## modification, are permitted provided that the following conditions are met: 
##
## * Redistributions of source code must retain the above copyright notice, 
## this list of conditions and the following disclaimer. 
## * Redistributions in binary form must reproduce the above copyright notice, 
## this list of conditions and the following disclaimer in the documentation 
## and/or other materials provided with the distribution. 
## * Neither name of Intel Corporation nor the names of its contributors 
## may be used to endorse or promote products derived from this software 
## without specific prior written permission.
## 
## THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
## "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
## LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR 
## A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL INTEL OR 
## CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, 
## EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, 
## PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR 
## PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY 
## OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
## NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS 
## SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
##
###########################################################################
##
## $Revision: 1.1.1.2 $
## $Date: 2001/06/15 00:21:33 $
##     


#Makefile for gena 
#	gena_client.c -> gena_client.o 
#	gena_server.c -> gena_server.o
#	gena_callback.c ->gena_callback.o

upnp_root = ../../

upnp_inc_dir = ../../inc

upnp_src_inc_dir = ../inc

upnp_lib_dir = ../lib

upnp_bin_dir = ../../bin

upnp_src_dir = ..

xerces_install = ../../xerces-c

xercyC_inc = $(upnp_inc_dir)/dom

xerces_lib = -L$(xerces_install)/lib -lxerces-c1_1 -lc

xercyC_lib_dir =$(upnp_src_dir)/dom

TARGET = $(upnp_lib_dir)/gena.o

CFLAGS += -I$(upnp_inc_dir) -I$(upnp_src_inc_dir) -I$(UUID_INC) -Wall -c -D_REENTRANT

ifeq ($(DEBUG),1)
CFLAGS += -g -O -DDEBUG
else
CFLAGS += -O2  -DNO_DEBUG
endif

ifeq ($(CLIENT),1)
CFLAGS += -DINCLUDE_CLIENT_APIS
endif

ifeq ($(DEVICE),1)
CFLAGS += -DINCLUDE_DEVICE_APIS
endif


all: gena_client.o gena_server.o  gena_callback.o $(TARGET) Makefile

$(TARGET): gena_client.o gena_server.o gena_callback.o
	$(LD) -r gena_client.o gena_server.o gena_callback.o -o $(TARGET)

clean: 	
	@if [ -f "$(TARGET)" ]; then rm $(TARGET); fi
	@rm -f *.o

gena_callback.o: gena_callback.c gena_client.c gena_server.c $(upnp_src_inc_dir)/gena/gena.h	
	$(GCC) -c $(CFLAGS) gena_callback.c  

gena_client.o: gena_client.c $(upnp_src_inc_dir)/gena/gena.h
	$(GCC) -c $(CFLAGS) gena_client.c 


gena_server.o: gena_server.c $(upnp_src_inc_dir)/gena/gena.h 
	$(GCC) -c $(CFLAGS) gena_server.c 




