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

MAKE = make
UPNP = bin/libupnp.a
SUBDIRS = src

VERSION=1.0.4

APACHE_LOCATION = ../apache_1.3.22

ifeq ($(DEBUG),1)
STRIPU =
else
STRIPU = strip $(UPNP)
endif

ifndef DEBUG
export DEBUG=0
endif

ifndef WEB
export WEB=1
endif

ifndef TOOLS
export TOOLS=1
endif

ifndef CLIENT
export CLIENT=1
endif

ifndef DEVICE
export DEVICE=1
endif

ifeq ($(DEVICE),0)
export WEB=0
endif

all: upnp


mod_upnp: APACHE_MODULE=1
	export APACHE_MODULE
mod_upnp: upnp

upnp:
	if [ ! -d bin ]; then mkdir bin; fi
	if [ ! -d src/lib ]; then mkdir src/lib; fi
	$(MAKE) -C src

doc: html pdf

html:
	if [ -d doc/html ]; then rm -rf doc/html; fi
	doc++ -v -nd -S -w -j --dir doc/html inc/upnp.h

pdf:
	doc++ -v -nd -S -w -j -t --package a4wide -o doc/upnpsdk.tex inc/upnp.h
	# Once to generate the basic document with undefined references
	cd doc; latex "\scrollmode\input upnpsdk.tex"; cd ..
	# Once to resolve the references
	cd doc; latex "\scrollmode\input upnpsdk.tex"; cd ..
	# Finally to fix up the references
	cd doc; latex "\scrollmode\input upnpsdk.tex"; cd ..
	dvips -o doc/upnpsdk.ps doc/upnpsdk.dvi
	cd doc; ps2pdf upnpsdk.ps; cd ..

clean:
	@set -e; for i in $(SUBDIRS); do $(MAKE) -C $$i clean; done
	@-rm -f bin/*.so
	@-rm -rf doc/html
	@if [ -f "doc/upnpsdk.tex" ]; then rm doc/upnpsdk.tex; fi
	@if [ -f "doc/upnpsdk.dvi" ]; then rm doc/upnpsdk.dvi; fi
	@if [ -f "doc/upnpsdk.ps"  ]; then rm doc/upnpsdk.ps; fi
	@if [ -f "doc/upnpsdk.pdf" ]; then rm doc/upnpsdk.pdf; fi
	@if [ -f "doc/upnpsdk.log" ]; then rm doc/upnpsdk.log; fi
	@if [ -f "doc/upnpsdk.aux" ]; then rm doc/upnpsdk.aux; fi

install: upnp
	@install -d /usr/include/upnp
	@install -d /usr/include/upnp/upnpdom
	@install -d /usr/include/upnp/tools
	@install bin/libupnp.so /usr/lib/libupnp.so.$(VERSION)
	ln -f -s /usr/lib/libupnp.so.$(VERSION) /usr/lib/libupnp.so
	@install inc/*.h /usr/include/upnp
	@install inc/upnpdom/*.h /usr/include/upnp/upnpdom
	@install inc/tools/*.h /usr/include/upnp/tools

mod_install: mod_upnp
	@install -d /usr/include/upnp
	@install -d /usr/include/upnp/upnpdom
	@install -d /usr/include/upnp/tools
	@install bin/libupnp.so /usr/lib/libupnp.so.$(VERSION)
	ln -f -s /usr/lib/libupnp.so.$(VERSION) /usr/lib/libupnp.so
	@install inc/*.h /usr/include/upnp
	@install inc/upnpdom/*.h /usr/include/upnp/upnpdom
	@install inc/tools/*.h /usr/include/upnp/tools
	@install -d $(APACHE_LOCATION)/src/modules/upnp
	@install src/inc/genlib/miniserver/miniserver.h $(APACHE_LOCATION)/src/modules/upnp
	@install src/inc/mod_upnp/mod_upnp.h $(APACHE_LOCATION)/src/modules/upnp
	@install src/apachelib/mod_upnp/mod_upnp.c $(APACHE_LOCATION)/src/modules/upnp
	@if ! grep -q libupnp $(APACHE_LOCATION)/src/Configuration.tmpl ; then \
	cat $(APACHE_LOCATION)/src/Configuration.tmpl | sed -e 's/EXTRA_LIBS.*/& \/usr\/lib\/libupnp.so\ \-lstdc\+\+/' > $(APACHE_LOCATION)/src/Configuration.tmpl.new ; \
	mv $(APACHE_LOCATION)/src/Configuration.tmpl $(APACHE_LOCATION)/src/Configuration.old ; \
	mv $(APACHE_LOCATION)/src/Configuration.tmpl.new $(APACHE_LOCATION)/src/Configuration.tmpl ; fi
	@if  ! grep -q mod_upnp $(APACHE_LOCATION)/src/Configuration.tmpl ; then \
	echo "AddModule modules/upnp/mod_upnp.o" >> $(APACHE_LOCATION)/src/Configuration.tmpl ; fi
	@touch $(APACHE_LOCATION)/src/modules/upnp/Makefile.tmpl

uninstall:
	@if [ -d /usr/include/upnp ]; then rm -rf /usr/include/upnp; fi
	@if [ -f /usr/lib/libupnp.so ]; then rm /usr/lib/libupnp.so; fi
	@if [ -f /usr/lib/libupnp.so.$(VERSION) ]; then rm /usr/lib/libupnp.so.$(VERSION); fi
	
