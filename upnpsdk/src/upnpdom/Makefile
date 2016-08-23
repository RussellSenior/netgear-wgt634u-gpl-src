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

OBJ = testc.o domCif.o Node.o NodeAct.o Parser.o NodeList.o Element.o Attr.o Document.o NamedNodeMap.o DOMException.o
CC = gcc
CCPP = g++
PLATFORM = LINUX
COMPILER = g++
CXXFLAGS = -w -O
CFLAGS += -Wall -D_REENTRANT  
PREFIX = /usr/local
LIBS =  
INCLUDE = -I ../../inc/upnpdom -I ../inc
lib_dir=../lib

all: $(lib_dir)/upnpdom.o

ifeq ($(DEBUG),1)
CFLAGS += -g -O -DDEBUG
else 
CFLAGS += -O2 -DNO_DEBUG
endif

ifeq ($(CLIENT),1)
CFLAGS += -DINCLUDE_CLIENT_APIS
endif

ifeq ($(DEVICE),1)
CFLAGS += -DINCLUDE_DEVICE_APIS
endif

$(lib_dir)/upnpdom.o: domCif.o Node.o NodeAct.o Parser.o NodeList.o Element.o Attr.o Document.o NamedNodeMap.o DOMException.o
	$(LD) -r domCif.o Node.o NodeAct.o Parser.o NodeList.o Element.o Attr.o Document.o NamedNodeMap.o DOMException.o -o $(lib_dir)/upnpdom.o

test: testc.o domCif.o Node.o NodeAct.o Parser.o NodeList.o Element.o Attr.o Document.o NamedNodeMap.o DOMException.o
	$(CCPP) $(CFLAGS) $(LIBS) -o test testc.o /downloads/upnp/bin/libiupnp.so

testc.o: testc.c
	$(CCPP) $(CFLAGS) -c $(INCLUDE) testc.c

domCif.o: domCif.cpp
	$(CCPP) $(CFLAGS) -c $(INCLUDE) domCif.cpp

Node.o: Node.cpp
	$(CCPP) $(CFLAGS) -c $(INCLUDE) Node.cpp

NodeAct.o: NodeAct.cpp
	$(CCPP) $(CFLAGS) -c $(INCLUDE) NodeAct.cpp

Parser.o: Parser.cpp
	$(CCPP) $(CFLAGS) -c $(INCLUDE) Parser.cpp

NodeList.o: NodeList.cpp
	$(CCPP) $(CFLAGS) -c $(INCLUDE) NodeList.cpp

Element.o: Element.cpp
	$(CCPP) $(CFLAGS) -c $(INCLUDE) Element.cpp

Attr.o: Attr.cpp
	$(CCPP) $(CFLAGS) -c $(INCLUDE) Attr.cpp

Document.o: Document.cpp
	$(CCPP) $(CFLAGS) -c $(INCLUDE) Document.cpp

DOMException.o: DOMException.cpp
	$(CCPP) $(CFLAGS) -c $(INCLUDE) DOMException.cpp

NamedNodeMap.o: NamedNodeMap.cpp
	$(CCPP) $(CFLAGS) -c $(INCLUDE) NamedNodeMap.cpp

clean:
	@rm *.o -f
	@rm $(lib_dir)/upnpdom.o -f 
