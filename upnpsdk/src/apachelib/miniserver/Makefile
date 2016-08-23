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

# $Revision: 1.1 $
# $Date: 2002/01/15 19:30:51 $


# makefile for /genlib/miniserver

inc_root=../../inc
ms_inc=$(inc_root)/genlib/miniserver
util_inc=$(inc_root)/genlib/util
http_inc=$(inc_root)/genlib/net/http
upnp_inc=../../../inc
xer_inc=../../../xerces-c/include
lib_dir=../../lib

objects = miniserver.o

# miniserver2 not used
#ifeq ($(WEB),1)
#objects += miniserver2.o
#endif

all: $(objects) $(lib_dir)/miniserverall.o

CFLAGS += -I $(inc_root) -I $(upnp_inc) -Wall -c -D_REENTRANT

ifeq ($(MODULE),1)
CFLAGS += -DINTERNAL_APACHE_MODULE
endif

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
CFLAGS += -g -O -DDEBUG
else 
CFLAGS += -O2 -DNO_DEBUG -DNDEBUG
endif

clean:
	@rm -f *.o
	@if [ -f $(lib_dir)/miniserverall.o ]; then rm $(lib_dir)/miniserverall.o -f; fi

miniserver.o: miniserver.cpp $(ms_inc)/miniserver.h $(inc_root)/genlib/tpool/scheduler.h \
		$(util_inc)/utilall.h $(util_inc)/genexception.h $(util_inc)/miscexceptions.h
	g++ $(CFLAGS) miniserver.cpp

miniserver2.o: miniserver2.cpp $(ms_inc)/miniserver2.h \
		$(http_inc)/parseutil.h $(http_inc)/readwrite.h
	g++ $(CFLAGS) miniserver2.cpp

$(lib_dir)/miniserverall.o: $(objects)
	ld -r  $(objects) -o $(lib_dir)/miniserverall.o
