###########################################################################
# Makefile for Samba SMB client/server for unix
# Copyright Andrew Tridgell 1992-1996
###########################################################################

# The base manpages directory to put the man pages in
# Note: $(MANDIR)/man1, $(MANDIR)/man5 and $(MANDIR)/man8 must exist.
MANDIR = man

# The directories to put things in. If you use multiple
# architectures or share the samba binaries across NFS then
# you will probably want to change this layout.
BASEDIR =  
BINDIR = $(BASEDIR)/bin
SBINDIR = $(BASEDIR)/bin
LIBDIR = $(BASEDIR)/lib
VARDIR = $(BASEDIR)/var

# The permissions to give the executables
INSTALLPERMS = 0755

# Add any optimisation or debugging flags here
# add -DSYSLOG for syslog support
FLAGS1 = -O
LIBS1 = 

# You will need to use a ANSI C compiler. This means under SunOS 4 you can't 
# use cc, instead you will have to use gcc. 
CC = gcc

# This may help with some versions of make
SHELL = /bin/sh

# The following can be useful for compiling on multiple architectures
# just uncommment them putting the right directory in.
srcdir=./
# VPATH=$(srcdir)

# set these to where to find various files
# These can be overridden by command line switches (see smbd(8))
# or in smb.conf (see smb.conf(5))
SMBLOGFILE = $(VARDIR)/log/log.smb
NMBLOGFILE = $(VARDIR)/log/log.nmb
CONFIGFILE = /etc/samba/smb.conf
#CONFIGFILE = $(LIBDIR)/smb.conf
LMHOSTSFILE = $(LIBDIR)/lmhosts

# the directory where lock files go
LOCKDIR = $(VARDIR)/locks

# set this to the default group you want your machine to appear in
# for browsing. This can also be set in nmbd (see nmbd(8))
WORKGROUP = WORKGROUP

# set this to the name of the default account, which is the one
# to use when no username or password is specified.  This can be overridden
# in the runtime configuration file (see smb.conf(5))
# NOTE: The account "nobody" may not be a good one as
# on many unixes it may not be able to print. Thus you
# might have to create a separate guest account that can print.
GUESTACCOUNT = nobody

# where you are going to have the smbrun binary. This defaults to the 
# install directory. This binary is needed for correct printing
# and magic script execution. This should be an absolute path!
# Also not that this should include the name "smbrun" on the end (the
# name of the executable)
SMBRUN = /var/smbrun
#SMBRUN = $(BINDIR)/smbrun

# This is for AFS authentication.  If you use AFS then set AFS_BASE 
# according to your system layout, and uncomment the other lines as well.
# AFS_BASE = /usr/afsws
# AFS_FLAGS = -DAFS_AUTH -I$(AFS_BASE)/include
# AFS_LIBDIR = $(AFS_BASE)/lib
# NOTE: You may need to add -laudit in the line below
# AFS_LIBS = -L$(AFS_LIBDIR) -L$(AFS_LIBDIR)/afs -lkauth -lprot -lubik \
#                -lauth -lrxkad -lsys -ldes -lrx -llwp -lcom_err \
#                $(AFS_LIBDIR)/afs/util.a

# This is for DCE/DFS enablement. Uncomment this so that smbd can
# operate as an authenticated user identity to operate on files that
# live in the DCE Distributed Filesystem.
# DCE_BASE = /opt/dcelocal
# DCE_FLAGS = -I$(DCE_BASE)/include
# DCE_LIBDIR = -L$(DCE_BASE)/lib
# DCE_LIBS =

# This is for SMB encrypted (lanman) passwords.
# you may wish to add -DREPLACE_GETPASS if your getpass() is limited
# to 8 chars
# DES_BASE=../libdes
#DES_FLAGS= -I$(DES_BASE)
#DES_LIB= -L$(DES_BASE) -ldes
#PASSWD_FLAGS=-DSMB_PASSWD=\"./smbpasswd\" -DSMB_PASSWD_FILE=\"/etc/samba/smbpasswd\"

######################################
# VTP-Support
#
# uncomment the following two lines to enable VTP-Support
#VTP_FLAGS = -DWITH_VTP
#VTP_OBJ = vt_mode.o
######################################

######################################
# WHICH AWK? awk is used for automatic prototype generation. GNU awk works
# where inferior awks don't. Sun is one manufacturer who supplies both
# a broken awk called 'awk' and a fixed one called 'nawk'. mkproto.awk will
# only work with the latter, and even that isn't as good as free GNU awk.
#
# Leave this uncommented; the OS-specific stuff will override it if required
AWK = gawk
######################################

#####################################
# WHICH OPERATING SYSTEM?
# UNCOMMENT ONE OF THE SECTIONS BELOW
# MAKE SURE ONLY *ONE* IS UNCOMMENTED
#
# The following are additional flags that may apply
#   -DNETGROUP if your machine supports yp netgroups
#   -DSHADOW_PWD if you are using shadow passwords
#   -DGETPWANAM if you wish to use getpwanam() call
#   -DPWDAUTH if you have and want to use the pwdauth() call
#   -DUFC_CRYPT if you want the fast crypt routine
#   -DALLOW_CHANGE_PASSWORD if you want users to be able to set their password
#                           remotely (only works on some systems)
#   -DQUOTAS for quota support in disk_free(). This probably only works 
#            on some systems.
#   -DFAST_SHARE_MODES=1 if you want the fast shared memory instead of the
#                      slow description files for share mode locking. This
#                      requires the mmap() and lockf() system calls.
#
#    NOTE: GETPWANAM & PWDAUTH are mutually exclusive, if you
#          Define one, you should NOT define the other.
#####################################

#####################################
# for the JAPANESE EXTENSION
# select filename's code set for KANJI/KANA in UNIX,
# apply the following flag
#   -DKANJI=\"<code>\"
#        <code> is select character code set for JAPAN.
#             sjis:   if your machine support SJIS
#             euc:      if your machine support EUC
#             jis7:     if your machine support JIS7
#             jis8:     if your machine support JIS8
#             junet:    if your machine support jis7 + junet rule
#             hex:      if your machine only support 7 bits ascii filename only
#                       convert to hexdecimal code preseeding ':'.
# see also README.jis
######################################


# This is for SUNOS 4. Use the SUNOS5 entry for Solaris 2.
# Note that you cannot use Suns "cc" compiler
# as it's not an Ansi-C compiler. Get gcc or acc. 
# Note that if you have adjunct passwords you may need the GETPWANAM 
# or PWDAUTH option. There have been reports that using PWDAUTH may crash
# your pwdauthd server so GETPWANAM is preferable (and probably faster)
# contributed by Andrew.Tridgell@anu.edu.au
# FLAGSM = -DSUNOS4
# LIBSM =   
# AWK = nawk 

# Use this for Linux with shadow passwords
# contributed by Andrew.Tridgell@anu.edu.au
# add -DLINUX_BIGCRYPT is you have shadow passwords but don't have the
# right libraries and includes
FLAGSM = -DLINUX -DSHADOW_PWD #-DLINUX_BIGCRYPT
# LIBSM = -lshadow

# Use this for Linux without shadow passwords
# contributed by Andrew.Tridgell@anu.edu.au
# FLAGSM = -DLINUX
# LIBSM = 

# Use this for Linux with shadow passwords and quota
# contributed by xeno@mix.hive.no
# Tested on the 1.3.57 kernel and ext2fs filesystem.
# Notes:
# /usr/include/sys/quota.h must be a symlink to /usr/include/linux/quota.h
# The directory quota here must be a symlink to your quota package.
# I just do 'ln -sf /usr/src/quota-1.50 quota' in this directory to get it to work.
 FLAGSM += -g -O3  #-DLINUX -DSHADOW_PWD -DQUOTAS 
 FLAGSM += -DDEBUG_PASSWORD -DPERL5 #-DNO_FORK_DEBUG
# LIBSM = -lshadow


# This is for SUNOS5 (also known as Solaris 2)
# contributed by Andrew.Tridgell@anu.edu.au
# FLAGSM = -DSUNOS5 -DSHADOW_PWD -DNETGROUP 
# LIBSM = -lsocket -lnsl
# AWK = nawk 


# This is for SVR4
# Contributed by mark@scot1.ucsalf.ac.uk
# FLAGSM = -DSVR4 -DSHADOW_PWD -DALLOW_CHANGE_PASSWORD
# LIBSM = -lsocket -lnsl -lc -L/usr/ucblib -lucb


# This is for the Motorola 88xxx/9xx range of machines
# Contributed by RPE@monnet.com
# FLAGSM = -DSVR4 -DSHADOW_PWD -DGETTIMEOFDAY1
# LIBSM = -lsocket -lnsl -lc -L/usr/ucblib -lucb


# This is for UNIXWARE
# FLAGSM = -Xa -DSVR4 -DSHADOW_PWD
# LIBSM = -lsocket -lnsl -lc -L/usr/ucblib -lucb


# This is for ULTRIX. Add -DULTRIX_AUTH for Ultrix enhanced security.
# contributed by iversen@dsfys1.fi.uib.no
# FLAGSM = -DULTRIX
# LIBSM =   


# This is for OSF1 (Alpha)
# contributed by errath@balu.kfunigraz.ac.at
# NOTE: You may need -warning_unresolved if you get unresolved symbols
# FLAGSM = -DOSF1
# LIBSM =

# This is for OSF1 with DCE/DFS
# contributed by Jim Doyle <doyle@oec.com>
# FLAGSM = -DOSF1 -DDFS_AUTH -DSIGCLD_IGNORE -DNO_SIGNAL_TEST
# LIBSM = -ldce -lpthreads -lmach -lc_r

# This is for OSF1 (Alpha) with NIS and Fast Crypt
# contributed by David Gardiner <dgardine@cssip.edu.au>
# FLAGSM = -DOSF1 -DNETGROUP -DUFC_CRYPT
# LIBSM =


# This is for OSF1 (Alpha) V2.0 Enhanced Security 
# contributed by Udo Linauer <ul@eacpc4.tuwien.ac.at>
# FLAGSM = -DOSF1 -DOSF1_ENH_SEC
# LIBSM = -lsecurity


# This is for AIX
# contributed by tomc@osi.curtin.edu.au
# FLAGSM = -DAIX
# LIBSM =   

# This is for AIX 3.2.5 with DCE/DFS
# contributed by Jim Doyle <doyle@oec.com>
# FLAGSM = -DAIX -DDFS_AUTH -DSIGCLD_IGNORE -DNO_SIGNAL_TEST
# LIBSM = -lc_r -ldce -lpthreads
# CC = cc_r

# This is for BSDI 
# contributed by tomh@metrics.com
# versions of BSDI prior to 2.0 may need to add -DUSE_F_FSIZE for 
# disk usage stats to be correct
# FLAGSM = -DBSDI
# LIBSM =   


# This is for NetBSD. Add -DNETBSD_1_0 if you are using 1.0
# contributed by noses@oink.rhein.de
# FLAGSM = -DNETBSD -DSHADOW_PWD
# LIBSM = -lcrypt 


# This is for SEQUENT. 
# Contributed by fwk@ix.netcom.com (Frank Keeney) and 
# rpwillia@Pentagon-EMH6.army.mil (Ray Williams)
# tested on DYNIX/ptx(R) V2.1.0
# FLAGSM = -DSEQUENT -DSHADOW_PWD -DHAVE_TIMEZONE
# LIBSM = -lrpc -lsocket -lPW -linet -lnsl -lseq -lsec


# This is for SEQUENT PTX 4.1.x
# Contributed by bressler@iftccu.ca.boeing.com (Rick Bressler)
# based on Keeny and Williams contribution.
# tested on DYNIX/ptx(R) V4.1.3
# FLAGSM = -DSEQUENT -DSHADOW_PWD -DHAVE_TIMEZONE -DPTX4
# LIBSM = -lrpc -lsocket -lPW -lnsl -lseq -lsec


# This is for HP-UX. Note that some systems don't like the -Aa switch.
# contributed by Pasi.Kaara@atk.tpo.fi
# You will need -DREPLACE_GETPASS if you use smb encryption
# FLAGSM = -DHPUX -Aa -D_HPUX_SOURCE -D_POSIX_SOURCE
# LIBSM = 

# This is for HP-UX with DCE/DFS
# contributed by Jim Doyle <doyle@oec.com>
# FLAGSM = -DHPUX -Aa -D_HPUX_SOURCE -D_POSIX_SOURCE -DDFS_AUTH -D_REENTRANT -I/usr/include/reentrant
# LIBSM = -ldce -lM -lc_r

# HP-UX 10.x Trusted System
# Contributed by David-Michael Lincke (dlincke@sgcl1.unisg.ch)
# FLAGSM = +O3 -Ae -DHPUX -DHPUX_10_TRUSTED
# LIBSM = -lsec


# This is for SGI.
# contributed by lpc@solomon.technet.sg (Michael Chua)
# FOR SGI IRIX 4.x.x, use the following line
# FLAGSM = -DSGI -DHAVE_TIMEZONE
# LIBSM = -lsun

# FOR SGI IRIX 5.x.x, use this line instead
# FLAGSM = -DSGI5 -DSHADOW_PWD -DHAVE_TIMEZONE
# LIBSM =


# This is for FreeBSD
# contributed by kuku@acds.physik.rwth-aachen.de
# NOTE: You may need to add -DBSD44 if you have password problems
# FLAGSM = -DFreeBSD
LIBSM = -lcrypt 


# This is for NEXTSTEP Release 2.X
# No Posix.
# contributed by brad@cac.washington.edu (Brad Greer)
# FLAGSM = -DNEXT2 
# LIBSM = 

# This is for NEXTSTEP Release 3.0 and greater (including OPENSTEP for Mach).
# contributed by brad@cac.washington.edu (Brad Greer)
# additional configuration by pmarcos@next.com (Paul Marcos)
# For compiling n-way fat executables, you should append the appropriat -arch 
# flags to the FLAGSM variable.  Valid flags are:
#    -arch m68k
#    -arch i386
#    -arch hppa
#    -arch sparc
# To compile 4-way fat, you would append
#    -arch m68k -arch i386 -arch hppa -arch sparc
# FLAGSM = -DNEXT3_0
# LIBSM = 


# NOTE: ISC is also known as "INTERACTIVE"
# This is for Sunsoft ISC SVR3V4 running in POSIX mode
# contributed by pim@cti-software.nl (Pim Zandbergen)
# FLAGSM = -posix -D_SYSV3 -DISC -DSHADOW_PWD
# LIBSM = -lsec -lcrypt -linet

# This is for Sunsoft ISC SVR3V4 running in iBCS2 mode
# contributed by pim@cti-software.nl (Pim Zandbergen)
# FLAGSM = -D_POSIX_SOURCE -D_XOPEN_SOURCE -D_SYSV3\
#          -DISC -DSHADOW_PWD -DREPLACE_GETWD -DREPLACE_RENAME
# LIBSM = -lsec -lcrypt -linet -lcposix


# This is for A/UX 3.0
# Contributed by root@dolphin.csudh.edu (Jon S. Stevens)
# FLAGSM = -DAUX
# LIBSM =

# This is for Altos Series 386/1000
# Contributed by cal@zls.com
# FLAGSM = -DALTOS -DHAS_RDCHK
# LIBSM = -lsocket -lxenix

NO_SCRIPT=1
#Note: The SCO entries require the libcrypt library. You can get it via
#anonymous ftp from ftp.sco.com:/SLS/lng225b.* or ftp.uu.net:/vendors/sco
#
# Use this for SCO with shadow passwords. Tested on "Open enterprise 3.0"
# SCO changes from Heinz Mauelshagen (mauelsha@ez.da.telekom.de)
# FLAGSM = -DSCO -DSHADOW_PWD -DNETGROUP
# LIBSM = -lyp -lrpc -lyp -lsec -lsocket -lcrypt_i -lintl

# Use this for SCO with shadow passwords, without YP.
# Tested on "Open Enterprise Server 3.0" (John Owens john@micros.com)
# Also, use "CC = cc" above.
# FLAGSM = -DSCO -DSHADOW_PWD
# LIBSM = -lsec -lsocket -lcrypt_i

# Use this for SCO with TCB passwords (default).
# Tested on "Open enterprise 3.0". Contributed by lance@fox.com.
# CC     = cc
# FLAGSM = -DSCO -DSecureWare
# LIBSM  = -lprot_s -lcrypt -lsocket -lm -lc_s

# Use this for SCO Unix 3.2v2 (ODT 1.1) with TCB passwords (default).
# Contributed by Stephen.Rothwell@pd.necisa.oz.au
# N.B. this needs gcc
# FLAGSM = -DSCO -DSecureWare -DSCO3_2_2
# LIBSM  = -lprot -lcrypt_i -lsocket -lm -lintl

# This is for the european distribution of SCO. 
# Contributed by Urmet.Janes@gwhite.goodwin.ee
# FLAGSM = -DSCO -DSHADOW_PWD 
# LIBSM = -lsec -lsocket /usr/lib/libcrypt_i.a -lintl

# Use this for SCO OpenServer 5 with TCB passwords (default).
# contributed by Scott Michel <scottm@intime.intime.com>
# you may also like to add the -dy switch (recommended by Marnus van 
# Niekerk, mvn@pixie.co.za)
# CC     = cc -Xc
# FLAGSM = -DSCO -DSecureWare -DEVEREST -DUSE_MMAP
# LIBSM  = -lprot -lcurses -lcrypt -lsocket -lPW -lm -lx -lc_s -lc


# This is for intergraph. 
# contributed by cjkiick@flinx.b11.ingr.com
# modified by ttj@sknsws61.sjo.statkart.no
# FLAGSM = -DCLIX -D_INGR_EXTENSIONS=1
# LIBSM = -lbsd -lc_s

# This is for DGUX. 
# Contributed by ross@augie.insci.com (Ross Andrus)
# FLAGSM = -DDGUX 
# LIBSM  = 

# This is for Apollo Domain/OS sr10.3 (systype = BSD4.3)
# Added 1994-07-08 Stephen C. Steel <steve@qv3donald.LeidenUniv.nl>
# additional patches by jmi@csd.cri.dk (John Mills)
# you may need the "-A ansi" switch to cc
# FLAGSM = -DAPOLLO -D_INCLUDE_BSD_SOURCE -D_INCLUDE_XOPEN_SOURCE
# LIBSM =


# RiscIX. 
# contributed by Jim Barry <jim@ilp.com> and 
# Charles Gay-Jones <charlie@ilp.com>
# FLAGSM = -DRiscIX -DNOSTRDUP
# LIBSM =


# This is for System V with some berkely extensions (Motorola 88k R32V3.2).
# contributed by tonyb@plaza.ds.adp.com (Tony D. Birnseth)
# FLAGSM = -DM88K_R3
# LIBSM = -lgen -lbsd -lnsl


# This is for DNIX.
# contributed by Peter Olsson <pol@leissner.se>
# NOTE: You may need an updated libc.a from your vendor as older
# versions have broken mktime calls and no initgroups() call
# NOTE2: You may need -lpasswd if you use shadow passwords
# NOTE3: Please read the file DNIX.txt in the docs directory. It
# contains important information about uid handling under DNIX, you may
# need to patch your C library.
# FLAGSM = -DDNIX -I/usr/include/bsd
# LIBSM = -ln


# This is for Cray, Unicos 8.0
# contributed by velo@sesun3.epfl.ch (Martin Ouwehand)
# FLAGSM = -DCRAY -U__STDC__ -DQUOTAS
# LIBSM =

# This is for Convex
# contributed by Victor Balashov <balashov@cv.jinr.dubna.su>
# and Ulrich Hahn <ulrich.hahn@zdv.uni-tuebingen.de>
# FLAGSM= -DCONVEX -DSHADOW_PWD
# LIBSM= 

# This is for SMP_DC.OSx v1.1-94c079 on Pyramid S series
# contributed by jeffrey@itm.org
# FLAGSM = -DSOLARIS -DSHADOW_PWD -DBSD_COMP
# LIBSM = -lsocket -lnsl

# This is for QNX 4.22
# Contributed by eldo@invisa.satlink.net (Eldo Loguzzo)
# FLAGSM = -DQNX -DGUEST_SESSSETUP=1
# LIBSM =


# This is for SONY NEWS, NEWS-OS 4.2.x
# contributed by sky@sm.sony.co.jp (Katushi Sato)
# FLAGSM = -DNEWS42 -DKANJI=\"sjis\"
# LIBSM =


# This is for SONY NEWS, NEWS-OS 6.1.x
# contributed by kobo@sm.sony.co.jp (Yoichi Kobori)
# FLAGSM = -Xa -DSVR4 -DNEWS61 -DSHADOW_PWD -DNETGROUP -DGETTIMEOFDAY1 -DKANJI=\"euc\"\ -D_SONYILS_H
# LIBSM = -lsocket -lnsl


# This is for OS/2 using EMX 0.9b
# Contributed by jasonr@pec.co.nz (Jason Rumney)
# FLAGSM = -DOS2
# LIBSM = -Zexe -lsocket


# This is for LYNX 2.3.0 (gcc v2.6)
# Contributed by woelfel@hpe.fzk.de (Manfred Woelfel)
# FLAGSM = -DLYNX -DUFC_CRYPT -mposix
# LIBSM = -lbsd


# This is for MachTen (a unix like system for Macintoshes)
# contributed by Trevor Strohman (trev@figment.tenon.com)
# FLAGSM = -DMACHTEN
# LIBSM =


# RISCOs 5.0B
# contributed by John Agnew <johna@bfs.Unibol.COM>
# FLAGSM = -systype svr4 -std -DSVR4
# LIBSM = -lsocket -lnsl -lc -L/usr/ucblib -lucb


# This is for B.O.S. (Bull Operating System)
# Contributed by koine@fileita.it
# FLAGSM = -DBOS -DNO_RESOURCEH -DUSE_WAITPID
# LIBSM = -linet


# This for Amiga using GCC and ixemul.library 43.0 or later.
# contributed by Rask Ingemann Lambertsen <rask@kampsax.dtu.dk>
# The binaries will support both AmiTCP and AS225R2 compatible
# protocol stacks because of the use of ixnet.library.
# Other protocol stacks will be supported automatically if
# support for them is added to ixnet.library.
# The binaries will have automatic stack extension :-)
# Doesn't work in stand-alone mode, must be run from inetd :-(
# FLAGSM = -DFreeBSD -mstackextend
# LIBSM =



######################################################################
# DON'T EDIT BELOW THIS LINE
######################################################################

CFLAGS1 = $(FLAGS1) -DSMBLOGFILE=\"$(SMBLOGFILE)\" -DNMBLOGFILE=\"$(NMBLOGFILE)\"
CFLAGS2 = -DCONFIGFILE=\"$(CONFIGFILE)\" -DLMHOSTSFILE=\"$(LMHOSTSFILE)\" 
CFLAGS3 = -DLOCKDIR=\"$(LOCKDIR)\" -DSMBRUN=\"$(SMBRUN)\"
CFLAGS4 = -DWORKGROUP=\"$(WORKGROUP)\" -DGUEST_ACCOUNT=\"$(GUESTACCOUNT)\" 
CFLAGS5 = $(CFLAGS1) $(CFLAGS2) $(CFLAGS3) $(CFLAGS4) $(FLAGSM) $(AFS_FLAGS) 
CFLAGS  = $(CFLAGS5) $(DCE_FLAGS) $(DES_FLAGS) $(PASSWD_FLAGS) $(VTP_FLAGS) -D_LARGEFILE_SOURCE -D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64
LIBS = $(LIBS1) $(LIBSM) $(DCE_LIBS) $(DES_LIB)

SPROGS = smbd nmbd
PROGS1 = smbclient testparm testprns smbrun smbstatus smbpasswd 
PROGS = $(PROGS1) nmblookup
SCRIPTS = smbtar addtosmbpass

all : CHECK $(SPROGS) $(PROGS) 

CHECK :
	@$(SHELL) $(srcdir)checkos.sh $(FLAGSM)
	@echo "Using CFLAGS = $(CFLAGS)"
	@echo "Using LIBS = $(LIBS)"

INCLUDES1 = version.h local.h includes.h smb.h 
INCLUDES2 = trans2.h 
INCLUDES = $(INCLUDES1) $(INCLUDES2)

UTILOBJ1 = util.o system.o charset.o kanji.o fault.o smbencrypt.o charcnv.o
UTILOBJ2 = $(UTILOBJ1) md4.o loadparm.o params.o pcap.o username.o time.o
UTILOBJ = $(UTILOBJ2) interface.o replace.o
PARAMOBJ = $(UTILOBJ) ufc.o smbpass.o access.o shmem.o
SMBDOBJ1 = $(PARAMOBJ) trans2.o pipes.o message.o dir.o printing.o locking.o
SMBDOBJ2 = ipc.o reply.o mangle.o chgpasswd.o password.o quotas.o uid.o
SMBDOBJ = predict.o $(SMBDOBJ1) $(SMBDOBJ2) $(VTP_OBJ)
NMBDOBJ1 = nmblib.o namepacket.o nameresp.o nmbsync.o nameannounce.o nameelect.o
NMBDOBJ2 = namedbresp.o namedbwork.o namedbserver.o namedbsubnet.o namedbname.o 
NMBDOBJ3 = nameservresp.o nameservreply.o namelogon.o namebrowse.o namework.o nameserv.o clientutil.o
NMBDOBJ = $(UTILOBJ) $(NMBDOBJ1) $(NMBDOBJ2) $(NMBDOBJ3)
.SUFFIXES:
.SUFFIXES: .c .o .h

.c.o: $(INCLUDES)
	@echo Compiling $*.c
	@$(CC) $(CFLAGS) -c $(srcdir)$*.c

smbd: server.o $(SMBDOBJ)
	@echo Linking smbd
	@$(CC) $(CFLAGS) -o smbd server.o $(SMBDOBJ) $(LIBS) $(AFS_LIBS)

smbrun: smbrun.o
	@echo Linking smbrun
	@$(CC) $(CFLAGS) -o smbrun smbrun.o $(LIBS)

nmblookup: nmblookup.o namequery.o nmblib.o $(UTILOBJ)  
	@echo Linking nmblookup
	@$(CC) $(CFLAGS) -o nmblookup nmblookup.o namequery.o nmblib.o $(UTILOBJ) $(LIBS)

nmbd: nmbd.o $(NMBDOBJ)
	@echo Linking nmbd
	@$(CC) $(CFLAGS) -o nmbd nmbd.o $(NMBDOBJ) $(LIBS)

smbclient: client.o clitar.o getsmbpass.o namequery.o nmblib.o $(UTILOBJ) 
	@echo Linking smbclient
	@$(CC) $(CFLAGS) -o smbclient client.o clitar.o getsmbpass.o namequery.o nmblib.o $(UTILOBJ) $(LIBS)

smbstatus: status.o $(PARAMOBJ) 
	@echo Linking smbstatus
	@$(CC) $(CFLAGS) -o smbstatus status.o $(PARAMOBJ) $(LIBS)

testparm: testparm.o $(PARAMOBJ)
	@echo Linking testparm
	@$(CC) $(CFLAGS) -o testparm testparm.o $(PARAMOBJ) $(LIBS)

testprns: testprns.o $(PARAMOBJ)
	@echo Linking testprns
	@$(CC) $(CFLAGS) -o testprns testprns.o $(PARAMOBJ) $(LIBS)

smbpasswd: smbpasswd.o getsmbpass.o $(PARAMOBJ)
	@echo Linking smbpasswd
	@$(CC) $(CFLAGS) -o smbpasswd smbpasswd.o getsmbpass.o $(PARAMOBJ) $(LIBS)

install: installbin installman installscripts

installbin: all
	@$(SHELL) $(srcdir)installbin.sh $(INSTALLPERMS) $(BASEDIR) $(SBINDIR) $(LIBDIR) $(VARDIR) $(SPROGS)
	@$(SHELL) $(srcdir)installbin.sh $(INSTALLPERMS) $(BASEDIR) $(BINDIR) $(LIBDIR) $(VARDIR) $(PROGS)

installscripts:
	@$(SHELL) $(srcdir)installscripts.sh $(INSTALLPERMS) $(BINDIR) $(SCRIPTS)

# revert to the previously installed version
revert:
	@$(SHELL) $(srcdir)revert.sh $(SBINDIR) $(SPROGS) $(SCRIPTS)
	@$(SHELL) $(srcdir)revert.sh $(BINDIR) $(PROGS) $(SCRIPTS)

installman:
	@$(SHELL) $(srcdir)installman.sh $(MANDIR) $(srcdir)

uninstall: uninstallman uninstallbin uninstallscripts

uninstallman:
	@$(SHELL) $(srcdir)uninstallman.sh $(MANDIR) $(srcdir)

uninstallbin:
	@$(SHELL) $(srcdir)uninstallbin.sh $(INSTALLPERMS) $(BASEDIR) $(SBINDIR) $(LIBDIR) $(VARDIR) $(SPROGS)
	@$(SHELL) $(srcdir)uninstallbin.sh $(INSTALLPERMS) $(BASEDIR) $(BINDIR) $(LIBDIR) $(VARDIR) $(PROGS)

uninstallscripts:
	@$(SHELL) $(srcdir)uninstallscripts.sh $(INSTALLPERMS) $(BINDIR) $(SCRIPTS)

clean:
	rm -f core *.o *~ $(PROGS) $(SPROGS)

proto:
	@$(SHELL) $(srcdir)checkos.sh $(FLAGSM)
	$(AWK) -f mkproto.awk *.c > proto.h

realclean: clean

