CC=gcc


top_srcdir=..
srcdir=.


include ../Make.rules

RANLIB=ranlib
AR=ar

LIB_OBJS=pr_fnmatch.o sstrncpy.o strsep.o vsnprintf.o glibc-glob.o glibc-hstrerror.o glibc-mkstemp.o pr-syslog.o pwgrent.o getopt.o getopt1.o


all:
	@echo "Running make from top level directory."
	cd ../ && $(MAKE) all

Makefile: Makefile.in ../config.status
	cd ../ && ./config.status


.c.o:
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $<

libsupp.a: $(LIB_OBJS)
	$(AR) rc libsupp.a $(LIB_OBJS)
	$(RANLIB) libsupp.a

lib: libsupp.a

clean:
	rm -f *.o libsupp.a

depend:
	makedepend $(CPPFLAGS) -Y *.c 2>/dev/null
	makedepend $(CPPFLAGS) -Y -fMakefile.in *.c 2>/dev/null

# Dependancies written below
# DO NOT DELETE

getopt.o: getopt.h
getopt1.o: getopt.h
glibc-glob.o: ../config.h ../include/options.h ../include/glibc-glob.h
glibc-hstrerror.o: ../include/conf.h ../include/version.h ../config.h
glibc-hstrerror.o: ../include/default_paths.h ../include/options.h
glibc-hstrerror.o: ../include/libsupp.h ../include/glibc-glob.h
glibc-mkstemp.o: ../include/conf.h ../include/version.h ../config.h
glibc-mkstemp.o: ../include/default_paths.h ../include/options.h
glibc-mkstemp.o: ../include/libsupp.h ../include/glibc-glob.h
pr-syslog.o: ../include/conf.h ../include/version.h ../config.h
pr-syslog.o: ../include/default_paths.h ../include/options.h
pr_fnmatch.o: ../include/conf.h ../include/version.h ../config.h
pr_fnmatch.o: ../include/default_paths.h ../include/options.h
pr_fnmatch.o: ../include/libsupp.h ../include/glibc-glob.h pr_fnmatch_loop.c
pwgrent.o: ../include/conf.h ../include/version.h ../config.h
pwgrent.o: ../include/default_paths.h ../include/options.h
sstrncpy.o: ../include/conf.h ../include/version.h ../config.h
sstrncpy.o: ../include/default_paths.h ../include/options.h
strsep.o: ../include/conf.h ../include/version.h ../config.h
strsep.o: ../include/default_paths.h ../include/options.h
strsep.o: ../include/libsupp.h ../include/glibc-glob.h
vsnprintf.o: ../include/conf.h ../include/version.h ../config.h
vsnprintf.o: ../include/default_paths.h ../include/options.h
vsnprintf.o: ../include/libsupp.h ../include/glibc-glob.h
