# /////////////////////////////////////////////////////////////////////////
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
#/////////////////////////////////////////////////////////////////////////

# $Revision: 1.1.1.4 $
# $Date: 2001/06/15 00:21:33 $

# makefile for /genlib/net/http

inc_root=../../../inc
util_inc=$(inc_root)/genlib/util
net_inc=$(inc_root)/genlib/net
http_inc=$(inc_root)/genlib/net/http
u_inc=../../../../inc
xer_inc=../../../../xerces-c/include
lib_dir=../../../lib


#CFLAGS = -I $(inc_root) -I $(xer_inc) -I $(u_inc) -fpic -Wall -c -D_REENTRANT
CFLAGS += -I $(inc_root) -I $(u_inc) -Wall -c -D_REENTRANT

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

all: tokenizer.o parseutil.o readwrite.o statuscodes.o server.o $(lib_dir)/httpall.o

clean:
	@rm -f *.o
	@if [ -f "$(lib_dir)/httpall.o" ]; then rm $(lib_dir)/httpall.o -f; fi

tokenizer.o: tokenizer.cpp $(http_inc)/tokenizer.h $(net_inc)/netreader.h \
		$(util_inc)/genexception.h $(util_inc)/xstring.h $(util_inc)/utilall.h
	$(CXX) $(CFLAGS) tokenizer.cpp
	

# parser.o: parser.cc $(http_inc)/parser.h $(http_inc)/tokenizer.h \
#	$(util_inc)/utilall.h $(util_inc)/genexception.h \
#	$(util_inc)/xstring.h $(util_inc)/miscexceptions.h
#		g++ $(CFLAGS) parser.cc

parseutil.o: parseutil.cpp $(http_inc)/parseutil.h $(http_inc)/tokenizer.h \
	$(util_inc)/utilall.h $(util_inc)/genexception.h \
	$(util_inc)/xstring.h $(util_inc)/miscexceptions.h
		$(CXX) $(CFLAGS) parseutil.cpp

statuscodes.o: statuscodes.cpp $(http_inc)/statuscodes.h
	$(CXX) $(CFLAGS) statuscodes.cpp

readwrite.o: readwrite.cpp $(http_inc)/parseutil.h $(http_inc)/tokenizer.h \
	$(util_inc)/genexception.h $(util_inc)/miscexceptions.h
		$(CXX) $(CFLAGS) readwrite.cpp

server.o: server.cpp $(http_inc)/parseutil.h $(http_inc)/tokenizer.h \
	$(http_inc)/server.h \
	$(util_inc)/genexception.h $(util_inc)/miscexceptions.h
		$(CXX) $(CFLAGS) server.cpp

$(lib_dir)/httpall.o: tokenizer.o parseutil.o statuscodes.o readwrite.o \
		server.o
	$(LD) -r tokenizer.o parseutil.o statuscodes.o readwrite.o server.o \
		-o $(lib_dir)/httpall.o
		
