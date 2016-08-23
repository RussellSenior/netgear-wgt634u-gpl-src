CC=gcc


top_srcdir=..
srcdir=.


include ../Make.rules


all:
	@echo "Running make from top level directory."
	cd .. && $(MAKE) all

Makefile: Makefile.in ../config.status
	cd ../ && ./config.status


.c.o:
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $<

module_glue.c: module_glue.c.tmpl $(MODULE_OBJS)
	srcdir=$(srcdir) $(srcdir)/glue.sh $(MODULE_OBJS)

modules: $(MODULE_OBJS) module_glue.o

clean:
	rm -f *.o module_glue.c

depend:
	rm -f module_glue.c
	makedepend $(CPPFLAGS) -Y *.c 2>/dev/null
	makedepend $(CPPFLAGS) -Y -fMakefile.in *.c 2>/dev/null

# Dependancies automagically written below by makedepend

# DO NOT DELETE

mod_auth.o: ../include/conf.h ../include/version.h ../config.h
mod_auth.o: ../include/default_paths.h ../include/options.h ../include/pool.h
mod_auth.o: ../include/regexp.h ../include/proftpd.h ../include/class.h
mod_auth.o: ../include/netacl.h ../include/support.h ../include/sets.h
mod_auth.o: ../include/dirtree.h ../include/netio.h ../include/modules.h
mod_auth.o: ../include/response.h ../include/timers.h ../include/inet.h
mod_auth.o: ../include/child.h ../include/netaddr.h ../include/bindings.h
mod_auth.o: ../include/ident.h ../include/help.h ../include/feat.h
mod_auth.o: ../include/ftp.h ../include/log.h ../include/xferlog.h
mod_auth.o: ../include/scoreboard.h ../include/data.h ../include/libsupp.h
mod_auth.o: ../include/glibc-glob.h ../include/fsio.h ../include/mkhome.h
mod_auth.o: ../include/ctrls.h ../include/event.h ../include/pr-syslog.h
mod_auth.o: ../include/privs.h
mod_auth_file.o: ../include/conf.h ../include/version.h ../config.h
mod_auth_file.o: ../include/default_paths.h ../include/options.h
mod_auth_file.o: ../include/pool.h ../include/regexp.h ../include/proftpd.h
mod_auth_file.o: ../include/class.h ../include/netacl.h ../include/support.h
mod_auth_file.o: ../include/sets.h ../include/dirtree.h ../include/netio.h
mod_auth_file.o: ../include/modules.h ../include/response.h
mod_auth_file.o: ../include/timers.h ../include/inet.h ../include/child.h
mod_auth_file.o: ../include/netaddr.h ../include/bindings.h
mod_auth_file.o: ../include/ident.h ../include/help.h ../include/feat.h
mod_auth_file.o: ../include/ftp.h ../include/log.h ../include/xferlog.h
mod_auth_file.o: ../include/scoreboard.h ../include/data.h
mod_auth_file.o: ../include/libsupp.h ../include/glibc-glob.h
mod_auth_file.o: ../include/fsio.h ../include/mkhome.h ../include/ctrls.h
mod_auth_file.o: ../include/event.h ../include/pr-syslog.h
mod_auth_pam.o: ../include/conf.h ../include/version.h ../config.h
mod_auth_pam.o: ../include/default_paths.h ../include/options.h
mod_auth_pam.o: ../include/pool.h ../include/regexp.h ../include/proftpd.h
mod_auth_pam.o: ../include/class.h ../include/netacl.h ../include/support.h
mod_auth_pam.o: ../include/sets.h ../include/dirtree.h ../include/netio.h
mod_auth_pam.o: ../include/modules.h ../include/response.h
mod_auth_pam.o: ../include/timers.h ../include/inet.h ../include/child.h
mod_auth_pam.o: ../include/netaddr.h ../include/bindings.h ../include/ident.h
mod_auth_pam.o: ../include/help.h ../include/feat.h ../include/ftp.h
mod_auth_pam.o: ../include/log.h ../include/xferlog.h ../include/scoreboard.h
mod_auth_pam.o: ../include/data.h ../include/libsupp.h
mod_auth_pam.o: ../include/glibc-glob.h ../include/fsio.h ../include/mkhome.h
mod_auth_pam.o: ../include/ctrls.h ../include/event.h ../include/pr-syslog.h
mod_auth_pam.o: ../include/privs.h
mod_auth_unix.o: ../include/conf.h ../include/version.h ../config.h
mod_auth_unix.o: ../include/default_paths.h ../include/options.h
mod_auth_unix.o: ../include/pool.h ../include/regexp.h ../include/proftpd.h
mod_auth_unix.o: ../include/class.h ../include/netacl.h ../include/support.h
mod_auth_unix.o: ../include/sets.h ../include/dirtree.h ../include/netio.h
mod_auth_unix.o: ../include/modules.h ../include/response.h
mod_auth_unix.o: ../include/timers.h ../include/inet.h ../include/child.h
mod_auth_unix.o: ../include/netaddr.h ../include/bindings.h
mod_auth_unix.o: ../include/ident.h ../include/help.h ../include/feat.h
mod_auth_unix.o: ../include/ftp.h ../include/log.h ../include/xferlog.h
mod_auth_unix.o: ../include/scoreboard.h ../include/data.h
mod_auth_unix.o: ../include/libsupp.h ../include/glibc-glob.h
mod_auth_unix.o: ../include/fsio.h ../include/mkhome.h ../include/ctrls.h
mod_auth_unix.o: ../include/event.h ../include/pr-syslog.h ../include/privs.h
mod_cap.o: ../lib/libcap/include/sys/capability.h ../include/conf.h
mod_cap.o: ../include/version.h ../config.h ../include/default_paths.h
mod_cap.o: ../include/options.h ../include/pool.h ../include/regexp.h
mod_cap.o: ../include/proftpd.h ../include/class.h ../include/netacl.h
mod_cap.o: ../include/support.h ../include/sets.h ../include/dirtree.h
mod_cap.o: ../include/netio.h ../include/modules.h ../include/response.h
mod_cap.o: ../include/timers.h ../include/inet.h ../include/child.h
mod_cap.o: ../include/netaddr.h ../include/bindings.h ../include/ident.h
mod_cap.o: ../include/help.h ../include/feat.h ../include/ftp.h
mod_cap.o: ../include/log.h ../include/xferlog.h ../include/scoreboard.h
mod_cap.o: ../include/data.h ../include/libsupp.h ../include/glibc-glob.h
mod_cap.o: ../include/fsio.h ../include/mkhome.h ../include/ctrls.h
mod_cap.o: ../include/event.h ../include/pr-syslog.h ../include/privs.h
mod_core.o: ../include/conf.h ../include/version.h ../config.h
mod_core.o: ../include/default_paths.h ../include/options.h ../include/pool.h
mod_core.o: ../include/regexp.h ../include/proftpd.h ../include/class.h
mod_core.o: ../include/netacl.h ../include/support.h ../include/sets.h
mod_core.o: ../include/dirtree.h ../include/netio.h ../include/modules.h
mod_core.o: ../include/response.h ../include/timers.h ../include/inet.h
mod_core.o: ../include/child.h ../include/netaddr.h ../include/bindings.h
mod_core.o: ../include/ident.h ../include/help.h ../include/feat.h
mod_core.o: ../include/ftp.h ../include/log.h ../include/xferlog.h
mod_core.o: ../include/scoreboard.h ../include/data.h ../include/libsupp.h
mod_core.o: ../include/glibc-glob.h ../include/fsio.h ../include/mkhome.h
mod_core.o: ../include/ctrls.h ../include/event.h ../include/pr-syslog.h
mod_core.o: ../include/privs.h
mod_ctrls.o: ../include/conf.h ../include/version.h ../config.h
mod_ctrls.o: ../include/default_paths.h ../include/options.h
mod_ctrls.o: ../include/pool.h ../include/regexp.h ../include/proftpd.h
mod_ctrls.o: ../include/class.h ../include/netacl.h ../include/support.h
mod_ctrls.o: ../include/sets.h ../include/dirtree.h ../include/netio.h
mod_ctrls.o: ../include/modules.h ../include/response.h ../include/timers.h
mod_ctrls.o: ../include/inet.h ../include/child.h ../include/netaddr.h
mod_ctrls.o: ../include/bindings.h ../include/ident.h ../include/help.h
mod_ctrls.o: ../include/feat.h ../include/ftp.h ../include/log.h
mod_ctrls.o: ../include/xferlog.h ../include/scoreboard.h ../include/data.h
mod_ctrls.o: ../include/libsupp.h ../include/glibc-glob.h ../include/fsio.h
mod_ctrls.o: ../include/mkhome.h ../include/ctrls.h ../include/event.h
mod_ctrls.o: ../include/pr-syslog.h ../include/privs.h ../include/mod_ctrls.h
mod_log.o: ../include/conf.h ../include/version.h ../config.h
mod_log.o: ../include/default_paths.h ../include/options.h ../include/pool.h
mod_log.o: ../include/regexp.h ../include/proftpd.h ../include/class.h
mod_log.o: ../include/netacl.h ../include/support.h ../include/sets.h
mod_log.o: ../include/dirtree.h ../include/netio.h ../include/modules.h
mod_log.o: ../include/response.h ../include/timers.h ../include/inet.h
mod_log.o: ../include/child.h ../include/netaddr.h ../include/bindings.h
mod_log.o: ../include/ident.h ../include/help.h ../include/feat.h
mod_log.o: ../include/ftp.h ../include/log.h ../include/xferlog.h
mod_log.o: ../include/scoreboard.h ../include/data.h ../include/libsupp.h
mod_log.o: ../include/glibc-glob.h ../include/fsio.h ../include/mkhome.h
mod_log.o: ../include/ctrls.h ../include/event.h ../include/pr-syslog.h
mod_log.o: ../include/privs.h
mod_ls.o: ../include/conf.h ../include/version.h ../config.h
mod_ls.o: ../include/default_paths.h ../include/options.h ../include/pool.h
mod_ls.o: ../include/regexp.h ../include/proftpd.h ../include/class.h
mod_ls.o: ../include/netacl.h ../include/support.h ../include/sets.h
mod_ls.o: ../include/dirtree.h ../include/netio.h ../include/modules.h
mod_ls.o: ../include/response.h ../include/timers.h ../include/inet.h
mod_ls.o: ../include/child.h ../include/netaddr.h ../include/bindings.h
mod_ls.o: ../include/ident.h ../include/help.h ../include/feat.h
mod_ls.o: ../include/ftp.h ../include/log.h ../include/xferlog.h
mod_ls.o: ../include/scoreboard.h ../include/data.h ../include/libsupp.h
mod_ls.o: ../include/glibc-glob.h ../include/fsio.h ../include/mkhome.h
mod_ls.o: ../include/ctrls.h ../include/event.h ../include/pr-syslog.h
mod_site.o: ../include/conf.h ../include/version.h ../config.h
mod_site.o: ../include/default_paths.h ../include/options.h ../include/pool.h
mod_site.o: ../include/regexp.h ../include/proftpd.h ../include/class.h
mod_site.o: ../include/netacl.h ../include/support.h ../include/sets.h
mod_site.o: ../include/dirtree.h ../include/netio.h ../include/modules.h
mod_site.o: ../include/response.h ../include/timers.h ../include/inet.h
mod_site.o: ../include/child.h ../include/netaddr.h ../include/bindings.h
mod_site.o: ../include/ident.h ../include/help.h ../include/feat.h
mod_site.o: ../include/ftp.h ../include/log.h ../include/xferlog.h
mod_site.o: ../include/scoreboard.h ../include/data.h ../include/libsupp.h
mod_site.o: ../include/glibc-glob.h ../include/fsio.h ../include/mkhome.h
mod_site.o: ../include/ctrls.h ../include/event.h ../include/pr-syslog.h
mod_xfer.o: ../include/conf.h ../include/version.h ../config.h
mod_xfer.o: ../include/default_paths.h ../include/options.h ../include/pool.h
mod_xfer.o: ../include/regexp.h ../include/proftpd.h ../include/class.h
mod_xfer.o: ../include/netacl.h ../include/support.h ../include/sets.h
mod_xfer.o: ../include/dirtree.h ../include/netio.h ../include/modules.h
mod_xfer.o: ../include/response.h ../include/timers.h ../include/inet.h
mod_xfer.o: ../include/child.h ../include/netaddr.h ../include/bindings.h
mod_xfer.o: ../include/ident.h ../include/help.h ../include/feat.h
mod_xfer.o: ../include/ftp.h ../include/log.h ../include/xferlog.h
mod_xfer.o: ../include/scoreboard.h ../include/data.h ../include/libsupp.h
mod_xfer.o: ../include/glibc-glob.h ../include/fsio.h ../include/mkhome.h
mod_xfer.o: ../include/ctrls.h ../include/event.h ../include/pr-syslog.h
mod_xfer.o: ../include/privs.h
