# configure options
%define 	with_snmp	0
%define		with_vtysh	1
%define		with_ospf_te	1
%define		with_nssa	1
%define		with_opaque_lsa 1
%define		with_tcp_zebra	0
%define		with_vtysh	1
%define		with_pam	1
%define		with_ipv6	1
%define		with_ospfclient 1
%define		with_ospfapi	1
%define		with_multipath	64
%define		quagga_user	quagga
%define		vty_group	quaggavty

# path defines
%define		_sysconfdir	/etc/quagga
%define		zeb_src		%{_builddir}/%{name}-%{version}
%define		zeb_rh_src	%{zeb_src}/redhat
%define		zeb_docs	%{zeb_src}/doc

# defines for configure
%define		_libexecdir	%{_exec_prefix}/libexec/quagga
%define		_includedir	%{_prefix}/include/quagga
%define		_libdir		%{_exec_prefix}/%{_lib}/quagga
%define		_localstatedir	/var/run/quagga

Summary: Routing daemon
Name:		quagga
Version:	0.96.4
Release:	2004072201
License:	GPL
Group: System Environment/Daemons
Source0:	http://www.quagga.net/snapshots/cvs/%{name}-%{version}.tar.gz
URL:		http://www.quagga.net
%if %with_snmp
#BuildRequires:	ucd-snmp-devel
Prereq:		ucd-snmp
%endif
%if %with_vtysh
BuildRequires:	readline readline-devel ncurses ncurses-devel
Prereq:		readline ncurses
%endif
BuildRequires:	texinfo tetex autoconf pam-devel patch
# Initscripts > 5.60 is required for IPv6 support
Prereq:		initscripts >= 5.60
Prereq:		ncurses readline pam
Prereq:		/sbin/install-info
Provides:	routingdaemon
BuildRoot:	%{_tmppath}/%{name}-%{version}-root
Obsoletes:	bird gated mrt zebra

%description
Quagga is a free software that manages TCP/IP based routing
protocol. It takes multi-server and multi-thread approach to resolve
the current complexity of the Internet.

Quagga supports BGP4, BGP4+, OSPFv2, OSPFv3, RIPv1, RIPv2, and RIPng.

Quagga is intended to be used as a Route Server and a Route Reflector. It is
not a toolkit, it provides full routing power under a new architecture.
Quagga by design has a process for each protocol.

Quagga is a fork of GNU Zebra.

%package contrib
Summary: contrib tools for quagga
Group: System Environment/Daemons

%description contrib
Contributed/3rd party tools which may be of use with quagga.

%package devel
Summary: Header and object files for quagga development
Group: System Environment/Daemons

%description devel
The quagga-devel package contains the header and object files neccessary for
developing OSPF-API and quagga applications.

%prep
%setup  -q

%build
%configure \
%if %with_ipv6
	--enable-ipv6 \
%endif
%if %with_snmp
	--enable-snmp \
%endif
%if %with_multipath
	--enable-multipath=%with_multipath \
%endif
%if %with_tcp_zebra
	--enable-tcp-zebra \
%endif
%if %with_nssa
	--enable-nssa \
%endif
%if %with_opaque_lsa
	--enable-opaque-lsa \
%endif
%if %with_ospf_te
	--enable-ospf-te \
%endif
%if %with_vtysh
	--enable-vtysh \
%endif
%if %with_ospfclient 
	--enable-ospfclient=yes \
%else
	--enable-ospfclient=no\
%endif
%if %with_ospfapi
	--enable-ospfapi=yes \
%else
	--enable-ospfapi=no \
%endif
%if %with_pam
	--with-libpam \
%endif
%if %quagga_user
	--enable-user=%quagga_user \
	--enable-group=%quagga_user \
%endif
%if %vty_group
	--enable-vty-group=%vty_group \
%endif
--enable-netlink

make %{?_smp_mflags} MAKEINFO="makeinfo --no-split"

pushd doc
texi2html quagga.texi
popd

%install
rm -rf $RPM_BUILD_ROOT

install -d $RPM_BUILD_ROOT/etc/{rc.d/init.d,sysconfig,logrotate.d,pam.d} \
	$RPM_BUILD_ROOT/var/log/quagga $RPM_BUILD_ROOT%{_infodir}

make install \
	DESTDIR=$RPM_BUILD_ROOT

# Remove this file, as it is uninstalled and causes errors when building on RH9
rm -rf $RPM_BUILD_ROOT/usr/share/info/dir

install %{zeb_rh_src}/zebra.init $RPM_BUILD_ROOT/etc/rc.d/init.d/zebra
install %{zeb_rh_src}/bgpd.init $RPM_BUILD_ROOT/etc/rc.d/init.d/bgpd
%if %with_ipv6
install %{zeb_rh_src}/ospf6d.init $RPM_BUILD_ROOT/etc/rc.d/init.d/ospf6d
install %{zeb_rh_src}/ripngd.init $RPM_BUILD_ROOT/etc/rc.d/init.d/ripngd
%endif
install %{zeb_rh_src}/ospfd.init $RPM_BUILD_ROOT/etc/rc.d/init.d/ospfd
install %{zeb_rh_src}/ripd.init $RPM_BUILD_ROOT/etc/rc.d/init.d/ripd
install -m644 %{zeb_rh_src}/quagga.pam $RPM_BUILD_ROOT/etc/pam.d/quagga
install -m644 %{zeb_rh_src}/quagga.logrotate $RPM_BUILD_ROOT/etc/logrotate.d/quagga
install -d -m750  $RPM_BUILD_ROOT/var/run/quagga

%pre
# add vty_group
%if %vty_group
groupadd -r %vty_group 2> /dev/null || :
%endif
# add quagga user and group
%if %quagga_user
/usr/sbin/useradd -M -r -s /bin/false -c "Quagga routing suite" \
 -d %_localstatedir %quagga_user 2> /dev/null || :
%endif

%post
# zebra_spec_add_service <service name> <port/proto> <comment>
# e.g. zebra_spec_add_service zebrasrv 2600/tcp "zebra service"

zebra_spec_add_service ()
{
  # Add port /etc/services entry if it isn't already there 
  if [ -f /etc/services ] && ! grep -q "^$1[^a-zA-Z0-9]" /etc/services ; then
    echo "$1		$2			# $3"  >> /etc/services
  fi
}

zebra_spec_add_service zebrasrv 2600/tcp "zebra service"
zebra_spec_add_service zebra    2601/tcp "zebra vty"
zebra_spec_add_service ripd     2602/tcp "RIPd vty"
%if %with_ipv6
zebra_spec_add_service ripngd   2603/tcp "RIPngd vty"
%endif
zebra_spec_add_service ospfd    2604/tcp "OSPFd vty"
zebra_spec_add_service bgpd     2605/tcp "BGPd vty"
%if %with_ipv6
zebra_spec_add_service ospf6d   2606/tcp "OSPF6d vty"
%endif
%if %with_ospfapi
zebra_spec_add_service ospfapi  2607/tcp "OSPF-API"
%endif

/sbin/chkconfig --add zebra 
/sbin/chkconfig --add ripd
%if %with_ipv6
/sbin/chkconfig --add ripngd
/sbin/chkconfig --add ospf6d
%endif
/sbin/chkconfig --add ospfd
/sbin/chkconfig --add bgpd

/sbin/install-info %{_infodir}/quagga.info.gz %{_infodir}/dir

# Create dummy files if they don't exist so basic functions can be used.
if [ ! -e %{_sysconfdir}/zebra.conf ]; then
	echo "hostname `hostname`" > %{_sysconfdir}/zebra.conf
	chmod 640 %{_sysconfdir}/zebra.conf
fi
if [ ! -e %{_sysconfdir}/vtysh.conf ]; then
	touch %{_sysconfdir}/vtysh.conf
	chmod 640 %{_sysconfdir}/vtysh.conf
fi

%postun
if [ "$1" -ge  "1" ]; then
	/etc/rc.d/init.d/zebra  condrestart >/dev/null 2>&1
	/etc/rc.d/init.d/ripd   condrestart >/dev/null 2>&1
%if %with_ipv6
	/etc/rc.d/init.d/ripngd condrestart >/dev/null 2>&1
%endif
	/etc/rc.d/init.d/ospfd  condrestart >/dev/null 2>&1
%if %with_ipv6
	/etc/rc.d/init.d/ospf6d condrestart >/dev/null 2>&1
%endif
	/etc/rc.d/init.d/bgpd   condrestart >/dev/null 2>&1
fi
/sbin/install-info --delete %{_infodir}/quagga.info.gz %{_infodir}/dir

%preun
if [ "$1" = "0" ]; then
        /sbin/chkconfig --del zebra
	/sbin/chkconfig --del ripd
%if %with_ipv6
	/sbin/chkconfig --del ripngd
%endif
	/sbin/chkconfig --del ospfd
%if %with_ipv6
	/sbin/chkconfig --del ospf6d
%endif
	/sbin/chkconfig --del bgpd
fi

%clean
#rm -rf $RPM_BUILD_ROOT

%files
%defattr(-,root,root)
%doc */*.sample* AUTHORS COPYING
%doc doc/quagga.html
%doc doc/mpls
%doc ChangeLog INSTALL NEWS README REPORTING-BUGS SERVICES TODO
%if %quagga_user
%dir %attr(751,%quagga_user,%quagga_user) %{_sysconfdir}
%dir %attr(750,%quagga_user,%quagga_user) /var/log/quagga 
%dir %attr(751,%quagga_user,%quagga_user) /var/run/quagga
%else
%dir %attr(750,root,root) %{_sysconfdir}
%dir %attr(750,root,root) /var/log/quagga
%dir %attr(755,root,root) /usr/share/info
%dir %attr(750,root,root) /var/run/quagga
%endif
%if %vty_group
%attr(750,%quagga_user,%vty_group) %{_sysconfdir}/vtysh.conf.sample
%endif
%{_infodir}/*info*
%{_mandir}/man*/*
%{_sbindir}/*
%if %with_vtysh
%{_bindir}/*
%endif
%config /etc/quagga/[!v]*
%config /etc/rc.d/init.d/*
%config(noreplace) /etc/pam.d/quagga
%config(noreplace) %attr(640,root,root) /etc/logrotate.d/*

%files contrib
%defattr(-,root,root)
%doc tools

%files devel
%defattr(-,root,root)
%dir %{_libdir}/*
%dir %{_includedir}/*.h
%dir %{_includedir}/ospfd/*
%if %with_ospfapi
%dir %{_includedir}/ospfapi/*
%endif

%changelog
* Sun Nov 2 2003 Paul Jakma <paul@dishone.st>
- Fix -devel package to include all files
- Sync to 0.96.4

* Tue Aug 12 2003 Paul Jakma <paul@dishone.st>
- Renamed to Quagga
- Sync to Quagga release 0.96

* Tue Mar 20 2003 Paul Jakma <paul@dishone.st>
- zebra privileges support

* Mon Mar 18 2003 Paul Jakma <paul@dishone.st>
- Fix mem leak in 'show thread cpu'
- Ralph Keller's OSPF-API
- Amir: Fix configure.ac for net-snmp

* Sat Mar 1 2003 Paul Jakma <paul@dishone.st>
- ospfd IOS prefix to interface matching for 'network' statement
- temporary fix for PtP and IPv6
- sync to zebra.org CVS

* Mon Jan 20 2003 Paul Jakma <paul@dishone.st>
- update to latest cvs
- Yon's "show thread cpu" patch - 17217
- walk up tree - 17218
- ospfd NSSA fixes - 16681
- ospfd nsm fixes - 16824
- ospfd OLSA fixes and new feature - 16823 
- KAME and ifindex fixes - 16525
- spec file changes to allow redhat files to be in tree

* Sat Dec 28 2002 Alexander Hoogerhuis <alexh@ihatent.com>
- Added conditionals for building with(out) IPv6, vtysh, RIP, BGP
- Fixed up some build requirements (patch)
- Added conditional build requirements for vtysh / snmp
- Added conditional to %files for %_bindir depending on vtysh

* Mon Nov 11 2002 Paul Jakma <paulj@alphyra.ie>
- update to latest CVS
- add Greg Troxel's md5 buffer copy/dup fix
- add RIPv1 fix
- add Frank's multicast flag fix

* Wed Oct 09 2002 Paul Jakma <paulj@alphyra.ie>
- update to latest CVS
- timestamped crypt_seqnum patch
- oi->on_write_q fix

* Mon Sep 30 2002 Paul Jakma <paulj@alphyra.ie>
- update to latest CVS
- add vtysh 'write-config (integrated|daemon)' patch
- always 'make rebuild' in vtysh/ to catch new commands

* Fri Sep 13 2002 Paul Jakma <paulj@alphyra.ie>
- update to 0.93b

* Wed Sep 11 2002 Paul Jakma <paulj@alphyra.ie>
- update to latest CVS
- add "/sbin/ip route flush proto zebra" to zebra RH init on startup

* Sat Aug 24 2002 Paul Jakma <paulj@alphyra.ie>
- update to current CVS
- add OSPF point to multipoint patch
- add OSPF bugfixes
- add BGP hash optimisation patch

* Fri Jun 14 2002 Paul Jakma <paulj@alphyra.ie>
- update to 0.93-pre1 / CVS
- add link state detection support
- add generic PtP and RFC3021 support
- various bug fixes

* Thu Aug 09 2001 Elliot Lee <sopwith@redhat.com> 0.91a-6
- Fix bug #51336

* Wed Aug  1 2001 Trond Eivind Glomsr�d <teg@redhat.com> 0.91a-5
- Use generic initscript strings instead of initscript specific
  ( "Starting foo: " -> "Starting $prog:" )

* Fri Jul 27 2001 Elliot Lee <sopwith@redhat.com> 0.91a-4
- Bump the release when rebuilding into the dist.

* Tue Feb  6 2001 Tim Powers <timp@redhat.com>
- built for Powertools

* Sun Feb  4 2001 Pekka Savola <pekkas@netcore.fi> 
- Hacked up from PLD Linux 0.90-1, Mandrake 0.90-1mdk and one from zebra.org.
- Update to 0.91a
- Very heavy modifications to init.d/*, .spec, pam, i18n, logrotate, etc.
- Should be quite Red Hat'isque now.