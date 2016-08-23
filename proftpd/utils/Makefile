CC=gcc


top_srcdir=..
srcdir=.


include ../Make.rules


all:
	@echo "Running make from top level directory."
	cd ../; $(MAKE) all

Makefile: Makefile.in ../config.status
	cd ../ && ./config.status


.c.o:
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $<

utils: $(FTPCOUNT_OBJS) $(FTPSHUT_OBJS) $(FTPTOP_OBJS) $(FTPWHO_OBJS)

clean:
	rm -f *.o

depend:
	makedepend $(CPPFLAGS) -Y *.c 2>/dev/null
	makedepend $(CPPFLAGS) -Y -fMakefile.in *.c 2>/dev/null

# Dependancies written below

# DO NOT DELETE

ftpcount.o: utils.h ../config.h ../include/version.h ../include/options.h
ftpcount.o: ../include/default_paths.h
ftpshut.o: ../include/conf.h ../include/version.h ../config.h
ftpshut.o: ../include/default_paths.h ../include/options.h ../include/pool.h
ftpshut.o: ../include/regexp.h ../include/proftpd.h ../include/class.h
ftpshut.o: ../include/netacl.h ../include/support.h ../include/sets.h
ftpshut.o: ../include/dirtree.h ../include/netio.h ../include/modules.h
ftpshut.o: ../include/response.h ../include/timers.h ../include/inet.h
ftpshut.o: ../include/child.h ../include/netaddr.h ../include/bindings.h
ftpshut.o: ../include/ident.h ../include/help.h ../include/feat.h
ftpshut.o: ../include/ftp.h ../include/log.h ../include/xferlog.h
ftpshut.o: ../include/scoreboard.h ../include/data.h ../include/libsupp.h
ftpshut.o: ../include/glibc-glob.h ../include/fsio.h ../include/mkhome.h
ftpshut.o: ../include/ctrls.h ../include/event.h ../include/pr-syslog.h
ftptop.o: utils.h ../config.h ../include/version.h ../include/options.h
ftptop.o: ../include/default_paths.h
ftpwho.o: utils.h ../config.h ../include/version.h ../include/options.h
ftpwho.o: ../include/default_paths.h
misc.o: ../include/conf.h ../include/version.h ../config.h
misc.o: ../include/default_paths.h ../include/options.h ../include/pool.h
misc.o: ../include/regexp.h ../include/proftpd.h ../include/class.h
misc.o: ../include/netacl.h ../include/support.h ../include/sets.h
misc.o: ../include/dirtree.h ../include/netio.h ../include/modules.h
misc.o: ../include/response.h ../include/timers.h ../include/inet.h
misc.o: ../include/child.h ../include/netaddr.h ../include/bindings.h
misc.o: ../include/ident.h ../include/help.h ../include/feat.h
misc.o: ../include/ftp.h ../include/log.h ../include/xferlog.h
misc.o: ../include/scoreboard.h ../include/data.h ../include/libsupp.h
misc.o: ../include/glibc-glob.h ../include/fsio.h ../include/mkhome.h
misc.o: ../include/ctrls.h ../include/event.h ../include/pr-syslog.h
scoreboard.o: utils.h ../config.h ../include/version.h ../include/options.h
scoreboard.o: ../include/default_paths.h
