#
# $Id: Makefile,v 1.4 2004/01/29 16:01:22 raven Exp $
#
-include ../Makefile.conf
include ../Makefile.rules

ifeq ($(LDAP), 1)
  SRCS = autofs-ldap-auto-master.c
  OBJS = autofs-ldap-auto-master.o
  BINS = autofs-ldap-auto-master
endif

CFLAGS += -I../include
LIBS = -lldap -llber

all: $(BINS) rc.autofs

autofs-ldap-auto-master: $(OBJS)
	$(CC) $(LDFLAGS) -o autofs-ldap-auto-master $(OBJS) $(LIBS)
	$(STRIP) autofs-ldap-auto-master

rc.autofs: rc.autofs.in
	sed -e "s|@@sbindir@@|$(sbindir)|g" \
	    -e "s|@@autofslibdir@@|$(autofslibdir)|g" \
	    -e "s|@@initdir@@|$(initdir)|g" < rc.autofs.in > rc.autofs

install: all
	install -d -m 755 $(INSTALLROOT)/etc
	-mv -f $(INSTALLROOT)/etc/auto.master $(INSTALLROOT)/etc/auto.master.old
	install -c auto.master -m 644 $(INSTALLROOT)/etc
	-mv -f $(INSTALLROOT)/etc/auto.misc $(INSTALLROOT)/etc/auto.misc.old
	install -c auto.misc   -m 644 $(INSTALLROOT)/etc
	install -c auto.net    -m 755 $(INSTALLROOT)/etc
	install -d -m 755 $(INSTALLROOT)/$(initdir)
	install -c rc.autofs   -m 755 $(INSTALLROOT)$(initdir)/autofs
	install -d -m 755 $(INSTALLROOT)$(autofslibdir)
ifeq ($(LDAP), 1)
	install -c autofs-ldap-auto-master -m 755 $(INSTALLROOT)$(autofslibdir)
endif

clean:
	rm -f *.o *.s autofs-ldap-auto-master rc.autofs

