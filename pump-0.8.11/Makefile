SBINPATH = $(RPM_BUILD_ROOT)/sbin
USRSBINPATH = $(sbindir)
USRLIBPATH = $(libdir)
INCPATH = $(includedir)
MAN8PATH = $(mandir)/man8
CFLAGS = $(DEB_CFLAGS) -I. -Wall -g $(RPM_OPT_FLAGS) -D__STANDALONE__ -DVERSION=\"$(VERSION)\"


LOADLIBES = /usr/lib/libpopt.a -lresolv
LDFLAGS = -g

VERSION=0.8.11
CVSTAG = r$(subst .,-,$(VERSION))

all:	pump netconfig

pump:	pump.o config.o libpump.a(dhcp.o)

netconfig:	net.o libpump.a(dhcp.o)
	$(CC) -o netconfig net.o libpump.a $(LOADLIBES) -lnewt -lslang

pump.o: pump.c pump.h

config.o: config.c pump.h

dhcp.o: dhcp.c pump.h

net.o: net.c net.h

clean:
	rm -f pump netconfig core *.o libpump.a 
	rm -f pump-*.tar.gz *~

install:
	mkdir -p $(SBINPATH) $(MAN8PATH) $(USRSBINPATH)
	mkdir -p $(USRLIBPATH) $(INCPATH)
	install -m 755 -s netconfig $(USRSBINPATH)/netconfig
	install -m 755 -s pump $(SBINPATH)/pump
	install -m 644 pump.8 $(MAN8PATH)
	install -m 644 libpump.a $(USRLIBPATH)
	install -m 644 pump.h $(INCPATH)

create-archive: tag-archive
	@rm -rf /tmp/pump
	@cd /tmp; cvs -Q -d $(CVSROOT) export -r$(CVSTAG) pump || echo GRRRrrrrr -- ignore [export aborted]
	@sed 's/VERSIONSUBST/$(VERSION)/' < /tmp/pump/pump.spec.in > /tmp/pump/pump.spec
	@rm /tmp/pump/pump.spec.in
	@mv /tmp/pump /tmp/pump-$(VERSION)
	@cd /tmp; tar czSpf pump-$(VERSION).tar.gz pump-$(VERSION)
	@rm -rf /tmp/pump-$(VERSION)
	@cp /tmp/pump-$(VERSION).tar.gz .
	@rm -f /tmp/pump-$(VERSION).tar.gz 
	@echo " "
	@echo "The final archive is ./pump-$(VERSION).tar.gz."

tag-archive:
	@cvs -Q tag -F $(CVSTAG)

archive: tag-archive create-archive
