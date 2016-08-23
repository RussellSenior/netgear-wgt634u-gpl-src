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

# $Revision: 1.1.1.3 $
# $Date: 2001/06/15 00:21:34 $

# makefile for genlib/tpool

inc_root=../../inc
u_inc=../../inc/genlib/util
p_inc=../../inc/genlib/tpool
lib_dir=../../lib
all: tpool.o scheduler.o interrupts.o $(lib_dir)/tpoolall.o

CFLAGS += -I $(inc_root) -Wall -c -D_REENTRANT

ifeq ($(DEBUG),1)
CFLAGS += -g -O -DDEBUG
else
CFLAGS += -O2 -DNO_DEBUG -DNDEBUG
endif

clean:
	@rm -f *.o
	@if [ -f "$(lib_dir)/tpoolall.o" ]; then rm $(lib_dir)/tpoolall.o; fi

tpool.o: tpool.cpp $(p_inc)/tpool.h $(p_inc)/scheduler.h $(u_inc)/utilall.h \
		$(u_inc)/genexception.h $(u_inc)/miscexceptions.h
	$(CXX) $(CFLAGS) tpool.cpp
	
scheduler.o: scheduler.cpp $(p_inc)/tpool.h $(p_inc)/scheduler.h
	$(CXX) $(CFLAGS) scheduler.cpp

interrupts.o: interrupts.cpp $(p_inc)/interrupts.h
	$(CXX) $(CFLAGS) interrupts.cpp

$(lib_dir)/tpoolall.o: tpool.o scheduler.o interrupts.o
	$(LD) -r tpool.o scheduler.o interrupts.o -o $(lib_dir)/tpoolall.o
