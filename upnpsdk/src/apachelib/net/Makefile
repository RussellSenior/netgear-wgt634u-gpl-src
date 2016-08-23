#####################################/
#
# Copyright (c) 2000 Intel Corporation
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
# * Redistributions of source code must retain the above copyright notice,
# this list of conditions and the following disclaimer.
# * Redistributions in binary form must reproduce the above copyright notice,
# this list of conditions and the following disclaimer in the documentation
# and/or other materials provided with the distribution.
# * Neither name of Intel Corporation nor the names of its contributors
# may be used to endorse or promote products derived from this software
# without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL INTEL OR
# CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
# EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
# PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
# PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
# OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
# NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
# SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#
# ////////////////////////////////////////////////////////////////////

# $Revision: 1.1 $
# $Date: 2002/01/15 19:30:51 $

# makefile for /genlib/net

inc_root=../../inc
util_inc=$(inc_root)/genlib/util
net_inc=$(inc_root)/genlib/net
str_inc=$(inc_root)/genlib/meta/stream
lib_dir=../../lib

CFLAGS += -I $(inc_root) -Wall -c -D_REENTRANT

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

# rules

all: netreader.o $(lib_dir)/netall.o Makefile

clean:
	@if [ -f *.o ]; then rm *.o -f; fi
	@if [ -f $(lib_dir)/netall.o ]; then rm $(lib_dir)/netall.o -f; fi

netreader.o: netreader.cpp $(net_inc)/netreader.h $(net_inc)/netexception.h \
		$(util_inc)/xstring.h $(util_inc)/genexception.h $(str_inc)/charreader.h
	g++ $(CFLAGS) netreader.cpp

$(lib_dir)/netall.o: netreader.o
	ld -r netreader.o -o $(lib_dir)/netall.o

