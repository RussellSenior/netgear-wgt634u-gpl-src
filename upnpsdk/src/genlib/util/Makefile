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

# $Revision: 1.1.1.5 $
# $Date: 2001/06/15 00:22:16 $

# makefile for /genlib/util

inc_root=../../inc
util_inc=../../inc/genlib/util
str_inc=$(inc_root)/genlib/meta/stream
lib_dir=../../lib

objects = genexception.o xstring.o memreader.o dbllist.o gmtdate.o membuffer.o


all: $(objects) $(lib_dir)/utilall.o

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

clean:
	@rm -f *.o
	@if [ -f "$(lib_dir)/utilall.o" ]; then rm $(lib_dir)/utilall.o; fi

membuffer.o: membuffer.c $(util_inc)/utilall.h $(util_inc)/membuffer.h 
	$(GCC) $(CFLAGS) -I ../../../inc membuffer.c

genexception.o: genexception.cpp $(util_inc)/utilall.h $(util_inc)/genexception.h
	$(CXX) $(CFLAGS) genexception.cpp

xstring.o: xstring.cpp $(util_inc)/utilall.h $(util_inc)/xstring.h \
		$(util_inc)/genexception.h $(util_inc)/miscexceptions.h
	$(CXX) $(CFLAGS) xstring.cpp

gmtdate.o: gmtdate.cpp $(util_inc)/util.h $(util_inc)/gmtdate.h
	$(CXX) $(CFLAGS) gmtdate.cpp

memreader.o: memreader.cpp $(util_inc)/memreader.h $(util_inc)/util.h $(str_inc)/charreader.h
	$(CXX) $(CFLAGS) memreader.cpp

dbllist.o: dbllist.cpp $(util_inc)/dbllist.h $(util_inc)/util.h
	$(CXX) $(CFLAGS) dbllist.cpp

$(lib_dir)/utilall.o: $(objects)
	$(LD) -r $(objects) -o $(lib_dir)/utilall.o

