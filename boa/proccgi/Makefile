# NEC KORVA/MARKHAM
#prefix = /usr/local
ifeq ($(USERLAND),)
USERLAND=$(HOME)/userland
endif
prefix = ${USERLAND}/ramdisk/ramfiles/usr/local
exec_prefix =	${prefix}
WEBDIR =	$(prefix)/www
CGIBINDIR =	$(WEBDIR)/cgi-bin
MANDIR =	${prefix}/man

CCOPT =		-O2
INCLS =		-I..
CFLAGS =	$(CCOPT) $(DEFS) $(INCLS)
LDFLAGS =
NETLIBS =	
INSTALL =	/usr/bin/install -c

CLEANFILES =	*.o proccgi



.c.o:
	@rm -f $@
	$(CC) $(CFLAGS) -c $*.c

all: proccgi

# NEC KORVA/MARKHAM
proccgi:	proccgi.o
	$(CC) $(LDFLAGS) $(STATICFLAG) $< $(LIBS) -o $@

clean:
	rm -f $(CLEANFILES)

distclean:
	rm -f $(CLEANFILES) Makefile
