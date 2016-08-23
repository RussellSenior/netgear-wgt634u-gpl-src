
DESTDIR=
KERNEL_HEADERS=-I/home2/vincent/work/usb-router-buildroot/build/linux-2.4.x/include

INSTALL=install -s

prefix=/usr
exec_prefix=${prefix}
bindir=${exec_prefix}/bin
sbindir=${exec_prefix}/sbin
mandir=${prefix}/man

SUBDIRS=libbridge brctl

all:

clean:

install:
	mkdir -p $(DESTDIR)$(mandir)/man8
	install -m 644 brctl.8 $(DESTDIR)$(mandir)/man8
