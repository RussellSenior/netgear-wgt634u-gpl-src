# Generated automatically from Makefile.in by configure.
#
# Standard e2fsprogs prologue....
#

srcdir = .
top_srcdir = ..
top_builddir = ..
my_dir = resize
INSTALL = /usr/bin/install -c
LDFLAG_STATIC = -static


# Beginning of file MCONFIG

all::

check::

SHELL = /bin/sh

prefix = /usr/local
root_prefix = /usr/local
exec_prefix = ${prefix}
root_bindir = $(root_prefix)/bin
root_sbindir = $(root_prefix)/sbin
root_libdir = $(root_prefix)/lib
bindir = ${exec_prefix}/bin
sbindir = ${exec_prefix}/sbin
libdir = ${exec_prefix}/lib
includedir = ${prefix}/include
mandir = ${prefix}/man
man1dir = $(mandir)/man1
man3dir = $(mandir)/man3
man8dir = $(mandir)/man8
infodir = ${prefix}/info
datadir = ${prefix}/share



INSTALL_PROGRAM = ${INSTALL}
INSTALL_DATA = ${INSTALL} -m 644
CC = gcc
BUILD_CC = gcc
DEFS =  -DENABLE_SWAPFS=1 -DPACKAGE=\"e2fsprogs\" -DVERSION=\"1.27\" -DSTDC_HEADERS=1 -DHAVE_ALLOCA_H=1 -DHAVE_ALLOCA=1 -DHAVE_UNISTD_H=1 -DHAVE_GETPAGESIZE=1 -DHAVE_LIMITS_H=1 -DHAVE_LOCALE_H=1 -DHAVE_NL_TYPES_H=1 -DHAVE_MALLOC_H=1 -DHAVE_STRING_H=1 -DHAVE_UNISTD_H=1 -DHAVE_SYS_PARAM_H=1 -DHAVE_GETCWD=1 -DHAVE_MUNMAP=1 -DHAVE_PUTENV=1 -DHAVE_SETENV=1 -DHAVE_SETLOCALE=1 -DHAVE_STRCHR=1 -DHAVE_STRCASECMP=1 -DHAVE_STRDUP=1 -DHAVE_STPCPY=1 -DHAVE_STPCPY=1 -DHAVE_LC_MESSAGES=1 -DHAVE_STDLIB_H=1 -DHAVE_UNISTD_H=1 -DHAVE_STDARG_H=1 -DHAVE_ERRNO_H=1 -DHAVE_MALLOC_H=1 -DHAVE_MNTENT_H=1 -DHAVE_PATHS_H=1 -DHAVE_DIRENT_H=1 -DHAVE_GETOPT_H=1 -DHAVE_SETJMP_H=1 -DHAVE_SIGNAL_H=1 -DHAVE_TERMIOS_H=1 -DHAVE_LINUX_FD_H=1 -DHAVE_LINUX_MAJOR_H=1 -DHAVE_SYS_IOCTL_H=1 -DHAVE_SYS_MOUNT_H=1 -DHAVE_SYS_SYSMACROS_H=1 -DHAVE_SYS_TIME_H=1 -DHAVE_SYS_STAT_H=1 -DHAVE_SYS_TYPES_H=1 -DHAVE_NET_IF_H=1 -DHAVE_NETINET_IN_H=1 -DHAVE_VPRINTF=1 -DHAVE_LSEEK64_PROTOTYPE=1 -DSIZEOF_SHORT=2 -DSIZEOF_INT=4 -DSIZEOF_LONG=4 -DSIZEOF_LONG_LONG=8 -DHAVE_GETRUSAGE=1 -DHAVE_LLSEEK=1 -DHAVE_LSEEK64=1 -DHAVE_OPEN64=1 -DHAVE_STRCASECMP=1 -DHAVE_SRANDOM=1 -DHAVE_FCHOWN=1 -DHAVE_FDATASYNC=1 -DHAVE_STRNLEN=1 
CFLAGS = 
CPPFLAGS = 
ALL_CFLAGS = $(CPPFLAGS) $(DEFS) $(USE_WFLAGS) $(CFLAGS) $(XTRA_CFLAGS) \
	-I$(top_builddir)/lib -I$(top_srcdir)/lib $(LINUX_INCLUDE) 
LDFLAGS = 
ALL_LDFLAGS = $(LDFLAGS)
RM = /bin/rm
LN = /bin/ln
LN_S = ln -s
MV = /bin/mv
CP = /bin/cp
CHMOD = /bin/chmod
AR = ar
AWK = /usr/bin/awk
SED = /bin/sed
PERL = /usr/bin/perl
RANLIB = ranlib
STRIP = strip
LD = $(PURE) gcc
ARUPD = $(AR) r
LDCONFIG = /sbin/ldconfig

#
# Library definitions
#
LIB = $(top_builddir)/lib
LIBSS = $(LIB)/libss.a
LIBCOM_ERR = $(LIB)/libcom_err.a
LIBE2P = $(LIB)/libe2p.a
LIBEXT2FS = $(LIB)/libext2fs.a
LIBUUID = $(LIB)/libuuid.a  
DEPLIBUUID = $(LIB)/libuuid.a

STATIC_LIBSS = $(LIB)/libss.a
STATIC_LIBCOM_ERR = $(LIB)/libcom_err.a
STATIC_LIBE2P = $(LIB)/libe2p.a
STATIC_LIBEXT2FS = $(LIB)/libext2fs.a
STATIC_LIBUUID = $(LIB)/libuuid.a  
DEPSTATIC_LIBUUID = $(LIB)/libuuid.a

PROFILED_LIBSS = $(LIB)/libss
PROFILED_LIBCOM_ERR = $(LIB)/libcom_err
PROFILED_LIBE2P = $(LIB)/libe2p
PROFILED_LIBEXT2FS = $(LIB)/libext2fs
PROFILED_LIBUUID = $(LIB)/libuuid  
DEPPROFILED_LIBUUID = $(LIB)/libuuid

#
# Use these definitions is you use tools 2.x, x < 16
#
#DLL_BIN=/usr/dll/bin
#JUMP_PREFIX=/usr/dll/jump/

#
# Use these definitions if you use tools 2.16 or above
#
DLL_BIN=/usr/bin
JUMP_PREFIX=/usr/bin/jump

# An include directive pointing to a directory holding enough linux-like
# include files to satisfy some programs here
LINUX_INCLUDE=

#
# A fast substitution command for fixing up man pages, shell scripts, etc.
#
SUBST_CONF=$(top_builddir)/util/subst.conf
SUBSTITUTE= $(top_builddir)/util/subst -f $(SUBST_CONF)
DEP_SUBSTITUTE= $(top_builddir)/util/subst $(SUBST_CONF)

$(top_builddir)/util/subst:
	cd $(top_builddir)/util ; $(MAKE) subst

#
# Warning flags
#
# Run make gcc-wall to do a build with warning messages.
#
#
WFLAGS=		-ansi -D_POSIX_SOURCE -pedantic \
			-Wall -Wwrite-strings -Wpointer-arith \
			-Wcast-qual -Wcast-align -Wtraditional \
			-Wstrict-prototypes -Wmissing-prototypes \
			-Wnested-externs -Winline -DNO_INLINE_FUNCS -Wshadow 

gcc-wall-new:
	(make USE_WFLAGS="$(WFLAGS)" > /dev/null) 2>&1 | sed -f $(top_srcdir)/util/gcc-wall-cleanup 

gcc-wall:
	make clean > /dev/null
	make gcc-wall-new

#
# Installation user and groups
#
BINGRP=		bin
BINOWN=		bin
BINMODE=	555
INCGRP=		bin
INCOWN=		bin
INCMODE=	444
LIBOWN=		bin
LIBGRP=		bin
LIBMODE=	444
MANGRP=		bin
MANOWN=		bin
MANMODE=	444

#
# Autoconf magic...
#

DEP_LIB_MAKEFILES = $(top_srcdir)/lib/Makefile.elf-lib  \
	$(top_srcdir)/lib/Makefile.dll-lib $(top_srcdir)/lib/Makefile.bsd-lib \
	$(top_srcdir)/lib/Makefile.checker $(top_srcdir)/lib/Makefile.profile

$(top_builddir)/config.status: $(top_srcdir)/configure
	cd $(top_builddir); ./config.status --recheck

$(top_builddir)/MCONFIG: $(top_srcdir)/MCONFIG.in $(top_builddir)/config.status
	cd $(top_builddir); CONFIG_FILES=MCONFIG ./config.status

$(top_builddir)/lib/substitute_sh: $(top_srcdir)/lib/substitute_sh.in \
		$(top_builddir)/config.status
	cd $(top_builddir); CONFIG_FILES=lib/substitute_sh ./config.status

$(top_builddir)/util/subst.conf: $(top_srcdir)/util/subst.conf.in \
		$(top_builddir)/config.status
	cd $(top_builddir); CONFIG_FILES=util/subst.conf ./config.status

Makefile: $(srcdir)/Makefile.in $(top_builddir)/MCONFIG \
		$(DEP_MAKEFILE) $(top_builddir)/config.status
	cd $(top_builddir); CONFIG_FILES=$(my_dir)/Makefile ./config.status

$(top_srcdir)/configure: $(top_srcdir)/configure.in
	cd $(top_srcdir) && autoconf

#
# Make depend magic...
#

.depend: Makefile $(SRCS) $(top_srcdir)/depfix.sed $(top_srcdir)/wordwrap.pl
	if test -n "$(SRCS)" ; then \
		$(CC) -M $(ALL_CFLAGS) $(SRCS) | \
			$(SED) -f $(top_srcdir)/depfix.sed \
			    -e 's; $(srcdir)/; $$(srcdir)/;g' \
			    -e 's; $(top_srcdir)/; $$(top_srcdir)/;g' \
			    -e 's; $(top_builddir)/; $$(top_builddir)/;g' \
			    -e 's; \./; ;g' \
			    -e '/^ *\\$$/d' | \
			$(PERL) $(top_srcdir)/wordwrap.pl > .depend; \
	else :; fi

depend:: .depend
	if test -n "$(SRCS)" ; then \
		sed -e '/^# +++ Dependency line eater +++/,$$d' \
			< $(srcdir)/Makefile.in | cat - .depend \
			> $(srcdir)/Makefile.in.new; \
	if cmp -s $(srcdir)/Makefile.in $(srcdir)/Makefile.in.new ; then \
		$(RM) $(srcdir)/Makefile.in.new ; \
	else \
		$(MV) $(srcdir)/Makefile.in $(srcdir)/Makefile.in.old; \
		$(MV) $(srcdir)/Makefile.in.new $(srcdir)/Makefile.in; \
	fi ; else :; fi

# End of file MCONFIG

PROGS=		resize2fs
TEST_PROGS=	test_extent
MANPAGES=	resize2fs.8

RESIZE_OBJS= extent.o resize2fs.o main.o sim_progress.o 

TEST_EXTENT_OBJS= extent.o test_extent.o

SRCS= $(srcdir)/extent.c \
	$(srcdir)/resize2fs.c \
	$(srcdir)/main.c \
	$(srcdir)/sim_progress.c

LIBS= $(LIBEXT2FS) $(LIBCOM_ERR)  
DEPLIBS= $(LIBEXT2FS) $(LIBCOM_ERR)

STATIC_LIBS= $(STATIC_LIBEXT2FS) $(STATIC_LIBCOM_ERR) 
STATIC_DEPLIBS= $(STATIC_LIBEXT2FS) $(STATIC_LIBCOM_ERR) 

.c.o:
	$(CC) -c $(ALL_CFLAGS) $< -o $@

all:: $(PROGS) $(TEST_PROGS) $(MANPAGES) 

resize2fs: $(RESIZE_OBJS) $(DEPLIBS)
	$(CC) $(ALL_LDFLAGS) -o resize2fs $(RESIZE_OBJS) $(LIBS)

resize2fs.static: $(RESIZE_OBJS)  $(STATIC_DEPLIBS)
	$(LD) $(ALL_LDFLAGS) $(LDFLAG_STATIC) -o resize2fs.static \
		$(RESIZE_OBJS) $(STATIC_LIBS) 

resize2fs.8: $(DEP_SUBSTITUTE) $(srcdir)/resize2fs.8.in
	$(SUBSTITUTE) $(srcdir)/resize2fs.8.in resize2fs.8

test_extent: $(TEST_EXTENT_OBJS)
	$(CC) $(ALL_LDFLAGS) -o test_extent $(TEST_EXTENT_OBJS) $(LIBS)
	
installdirs:
	$(top_srcdir)/mkinstalldirs $(DESTDIR)$(root_sbindir) \
		$(DESTDIR)$(man8dir)

install: $(PROGS) $(MANPAGES) installdirs
	for i in $(PROGS); do \
		$(INSTALL_PROGRAM) $$i $(DESTDIR)$(root_sbindir)/$$i; \
		$(STRIP) $(DESTDIR)$(root_sbindir)/$$i; \
	done
	for i in $(MANPAGES); do \
		$(RM) -f $(DESTDIR)$(man8dir)/$$i.gz; \
		$(INSTALL_DATA) $$i $(DESTDIR)$(man8dir)/$$i; \
	done

uninstall:
	for i in $(PROGS); do \
		$(RM) -f $(DESTDIR)$(root_sbindir)/$$i; \
	done
	for i in $(MANPAGES); do \
		$(RM) -f $(DESTDIR)$(man8dir)/$$i; \
	done

test_extent.out: test_extent $(srcdir)/test_extent.in
	LD_LIBRARY_PATH=$(LIB) ./test_extent < $(srcdir)/test_extent.in \
		> test_extent.out

check:: test_extent.out
	@if cmp -s test_extent.out $(srcdir)/test_extent.in ; then \
		echo "Test succeeded." ; \
	else \
		echo "Test failed!" ; \
		diff test_extent.out $(srcdir)/test_extent.in ; \
		exit 1 ; \
	fi
	
clean:
	$(RM) -f $(PROGS) $(TEST_PROGS) $(MANPAGES) \#* *.s *.o *.a *~ core \
		resize2fs.static test_extent.out

mostlyclean: clean

distclean: clean
	$(RM) -f .depend Makefile $(srcdir)/TAGS $(srcdir)/Makefile.in.old

#
# Kludge to create a "special" e2fsprogs distribution file.
#

SRCROOT = `echo e2fsprogs-1.27 | sed -e 's/-WIP//' \
			-e 's/pre-//' -e 's/-PLUS//'`
TAR=tar

$(top_srcdir)/.exclude-file:
	a=$(SRCROOT); \
	(cd $(top_srcdir)/.. ; find e2fsprogs \( -name \*~ -o -name \*.orig \
		-o -name CVS -o -name \*.rej \) -print) \
		| sed -e "s/e2fsprogs/$$a/" > $(top_srcdir)/.exclude-file
	echo "$(SRCROOT)/build" >> $(top_srcdir)/.exclude-file
	echo "$(SRCROOT)/rpm.log" >> $(top_srcdir)/.exclude-file
	echo "$(SRCROOT)/powerquest" >> $(top_srcdir)/.exclude-file
	echo "$(SRCROOT)/.exclude-file" >> $(top_srcdir)/.exclude-file
	echo $(SRCROOT)/e2fsprogs-1.27.tar.gz \
		>> $(top_srcdir)/.exclude-file
	echo $(SRCROOT)/e2fsprogs-ALL-1.27.tar.gz \
		>> $(top_srcdir)/.exclude-file
	
source_tar_file: $(top_srcdir)/.exclude-file
	(cd $(top_srcdir)/..; a=$(SRCROOT); rm -f $$a ; ln -sf e2fsprogs $$a ; \
		$(TAR) -c -h -v -f - \
			-X $$a/.exclude-file $$a | \
		gzip -9 > e2fsprogs-ALL-1.27.tar.gz)
	rm -f $(top_srcdir)/.exclude-file

# +++ Dependency line eater +++
# 
# Makefile dependencies follow.  This must be the last section in
# the Makefile.in file
#
extent.o: $(srcdir)/extent.c $(srcdir)/resize2fs.h \
 $(top_srcdir)/lib/ext2fs/ext2_fs.h $(top_builddir)/lib/ext2fs/ext2_types.h \
 $(top_srcdir)/lib/ext2fs/ext2fs.h $(top_srcdir)/lib/et/com_err.h \
 $(top_srcdir)/lib/ext2fs/ext2_io.h $(top_builddir)/lib/ext2fs/ext2_err.h \
 $(top_srcdir)/lib/ext2fs/bitops.h
resize2fs.o: $(srcdir)/resize2fs.c $(srcdir)/resize2fs.h \
 $(top_srcdir)/lib/ext2fs/ext2_fs.h $(top_builddir)/lib/ext2fs/ext2_types.h \
 $(top_srcdir)/lib/ext2fs/ext2fs.h $(top_srcdir)/lib/et/com_err.h \
 $(top_srcdir)/lib/ext2fs/ext2_io.h $(top_builddir)/lib/ext2fs/ext2_err.h \
 $(top_srcdir)/lib/ext2fs/bitops.h
main.o: $(srcdir)/main.c $(srcdir)/resize2fs.h \
 $(top_srcdir)/lib/ext2fs/ext2_fs.h $(top_builddir)/lib/ext2fs/ext2_types.h \
 $(top_srcdir)/lib/ext2fs/ext2fs.h $(top_srcdir)/lib/et/com_err.h \
 $(top_srcdir)/lib/ext2fs/ext2_io.h $(top_builddir)/lib/ext2fs/ext2_err.h \
 $(top_srcdir)/lib/ext2fs/bitops.h $(top_srcdir)/version.h
sim_progress.o: $(srcdir)/sim_progress.c $(srcdir)/resize2fs.h \
 $(top_srcdir)/lib/ext2fs/ext2_fs.h $(top_builddir)/lib/ext2fs/ext2_types.h \
 $(top_srcdir)/lib/ext2fs/ext2fs.h $(top_srcdir)/lib/et/com_err.h \
 $(top_srcdir)/lib/ext2fs/ext2_io.h $(top_builddir)/lib/ext2fs/ext2_err.h \
 $(top_srcdir)/lib/ext2fs/bitops.h
