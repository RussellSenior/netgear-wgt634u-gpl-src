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
## $Date: 2001/06/15 00:21:34 $
##     

#Makefile for service_table.c -> service_table.o

upnp_src_inc_dir = ../../inc

upnp_src_dir = ../..

upnp_inc_dir = ../../../inc

xerces_install = ../../../xerces-c-src_1_1_0



xercyC_inc = $(upnp_inc_dir)/dom

xercyC_lib_dir = $(upnp_src_dir)/dom

xerces_lib = -L$(xerces_install)/lib -lxerces-c1_1 -lc

lib_dir = $(upnp_src_dir)/lib

TARGET = $(lib_dir)/client_table.o

CFLAGS += -I$(upnp_src_inc_dir) -I$(upnp_inc_dir) -I$(xerces_install)/include -Wall -c

ifeq ($(WEB),1)
CFLAGS += -DINTERNAL_WEB_SERVER
endif

ifeq ($(CLIENT),1)
CFLAGS += -DINCLUDE_CLIENT_APIS
endif

ifeq ($(DEVICE),1)
CFLAGS += -DINCLUDE_DEVICE_APIS
endif

ifeq ($(DEBUG),1)
CFLAGS += -g -O -D_REENTRANT -DDEBUG
else
CFLAGS += -O2 -D_REENTRANT -DNO_DEBUG
endif

all: $(TARGET)

clean:
	@if [ -f $(TARGET) ]; then rm $(TARGET); fi
	@rm -f *.o

$(TARGET): client_table.c $(upnp_src_inc_dir)/genlib/client_table/client_table.h $(upnp_inc_dir)/upnp.h  
	$(GCC) $(CFLAGS) -c client_table.c  -o $(TARGET)
