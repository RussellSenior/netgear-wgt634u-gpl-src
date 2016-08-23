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

# $Revision: 1.1.1.2 $
# $Date: 2001/06/15 00:21:36 $

# makefile for genlib/urlconfig

inc_root=../inc
up_inc=../../inc
h_inc=$(inc_root)/genlib/net/http
u_inc=$(inc_root)/urlconfig

upnp_root=../../inc
genlib_r=../inc/
url_inc=../inc/urlconfig

lib_dir=../lib
all: urlconfig.o $(lib_dir)/urlconfigall.o

CFLAGS += -I $(upnp_root) -I $(genlib_r) -I $(url_inc) -Wall -c -D_REENTRANT

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
	@if [ -f "$(lib_dir)/urlconfigall.o" ]; then rm $(lib_dir)/urlconfigall.o; fi

urlconfig.o: urlconfig.cpp $(up_inc)/upnp.h $(up_inc)/upnpdom/domCif.h \
		$(h_inc)/parseutil.h $(u_inc)/urlconfig.h
	$(CXX) $(CFLAGS) urlconfig.cpp

$(lib_dir)/urlconfigall.o: urlconfig.o
	$(LD) -r urlconfig.o -o $(lib_dir)/urlconfigall.o
