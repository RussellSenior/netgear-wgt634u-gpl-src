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

TARGET=../../bin/libupnp.a

CC = gcc
CFLAGS += -Wall  $(DEFS) 
C2FLAGS = -Wall $(DEFS)
INCLUDES = -I ../inc -I ../../inc -I ../../inc/upnpdom  -I ../inc/tools -I$(UUID_INC)
LIBS = -lpthread -luuid

ifeq ($(DEBUG),1)
CFLAGS += -g -O -DDEBUG
C2FLAGS += -g -O -DDEBUG
else
CFLAGS += -O2
C2FLAGS += -O2
endif

objects = upnpapi.o config.o ../lib/ssdp.o ../lib/soap.o \
	  ../lib/miniserverall.o ../lib/service_table.o ../lib/tpoolall.o \
	  ../lib/http_client.o ../lib/client_table.o ../lib/utilall.o \
	  ../lib/gena.o ../lib/upnpdom.o ../lib/timer_thread.o ../lib/netall.o \
          ../lib/httpall.o ../lib/urlconfigall.o 

ifeq ($(WEB),1)
CFLAGS += -DINTERNAL_WEB_SERVER
endif

ifeq ($(TOOLS),1)
objects += upnptools.o
endif

ifeq ($(CLIENT),1)
CFLAGS += -DINCLUDE_CLIENT_APIS
endif

ifeq ($(DEVICE),1)
CFLAGS += -DINCLUDE_DEVICE_APIS
endif

default: $(TARGET)
all:	 $(TARGET)

$(TARGET): $(objects) makefile
#	$(CC) $(C2FLAGS) $(INCLUDES) $(objects) -o $(TARGET) $(LD_PATH) $(LIBS)
	$(AR) r $(TARGET) $(objects)
	ranlib $(TARGET)

%.o:	%.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $<


clean:
	@if [ -a "$(TARGET)" ]; then rm $(TARGET); fi
	@rm -f *.o
