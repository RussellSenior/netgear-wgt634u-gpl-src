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
## ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
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
#
# $Revision: 1.1.1.4 $
# $Date: 2001/06/15 00:22:14 $
#


CC=gcc
INCLUDES= -I../../inc  -I ../../inc/tools
LIBS= -lpthread  ../../bin/libupnp.so


ifeq ($(DEBUG),1)
OPT = -g -O2
else
OPT = -O2
endif

CFLAGS += -Wall $(OPT)

APPS = upnp_tv_device upnp_tv_ctrlpt

all: $(APPS)

upnp_tv_device: upnp_tv_device.o sample_util.o
	$(CC)  $(CFLAGS) upnp_tv_device.o  sample_util.o $(LIBS) -o  $@ 
	@echo "make $@ finished on `date`"

upnp_tv_ctrlpt: upnp_tv_ctrlpt.o sample_util.o
	$(CC)  $(CFLAGS) upnp_tv_ctrlpt.o sample_util.o  $(LIBS) -o  $@ 
	@echo "make $@ finished on `date`"

%.o:	%.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $<

clean:
	rm -f *.o $(APPS)


