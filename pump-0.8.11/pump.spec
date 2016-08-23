Summary: A Bootp and DHCP client for automatic IP configuration.
Name: pump
Version: 0.8.11
Release: 7
Group: System Environment/Daemons
License: MIT
BuildRoot: %{_tmppath}/%{name}-root
Source: pump-%{version}.tar.gz
Obsoletes: bootpc
BuildRequires: newt-devel
Requires: initscripts >= 3.92
Patch: pump-0.8.11-nobootp.patch
Patch1: pump-0.8.11-retry-forever.patch
Patch2: pump-0.8.11-21088.patch
Patch3: pump-0.8.11-17724.patch

%description
DHCP (Dynamic Host Configuration Protocol) and BOOTP (Boot Protocol)
are protocols which allow individual devices on an IP network to get
their own network configuration information (IP address, subnetmask,
broadcast address, etc.) from network servers. The overall purpose of
DHCP and BOOTP is to make it easier to administer a large network.

Pump is a combined BOOTP and DHCP client daemon, which allows your
machine to retrieve configuration information from a server. You
should install this package if you are on a network which uses BOOTP
or DHCP.

%package devel
Summary: Development tools for sending DHCP and BOOTP requests.
Group: Development/Libraries

%description devel
The pump-devel package provides system developers the ability to send
BOOTP and DHCP requests from their programs. BOOTP and DHCP are
protocols used to provide network configuration information to
networked machines.

%package -n netconfig
Group: Applications/System
Summary: A text-based tool for simple configuration of ethernet devices.

%description -n netconfig
A text-based tool for simple configuration of ethernet devices.


%prep
%setup -q
%patch -p1
%patch1 -p1
%patch2 -p1
%patch3 -p1

%build
make

%install
rm -rf $RPM_BUILD_ROOT

%makeinstall RPM_BUILD_ROOT=$RPM_BUILD_ROOT

%clean
rm -rf $RPM_BUILD_ROOT

%files
%defattr(-,root,root)
/sbin/pump
%{_mandir}/*/*

%files devel
%defattr(-,root,root)
%{_libdir}/libpump.a
%{_includedir}/pump.h

%files -n netconfig
%defattr(-,root,root)
%{_sbindir}/netconfig

%changelog
* Sun Aug 26 2001 Elliot Lee <sopwith@redhat.com>
- Fix one half #17724

* Wed Jul 25 2001 Trond Eivind Glomsrød <teg@redhat.com>
- Don't obsolete netconfig, it's now separate again

* Tue Jul 24 2001 Elliot Lee <sopwith@redhat.com>
- Fix installer segfaults with nobootp patch.

* Mon Jul 23 2001 Trond Eivind Glomsrød <teg@redhat.com>
- split out netconfig to a package of its own

* Thu Jul 19 2001 Elliot Lee <sopwith@redhat.com>
- Patch from bugs #19501, #19502, #21088, etc.

* Sun Jun 24 2001 Elliot Lee <sopwith@redhat.com>
- Bump release + rebuild.

* Thu Mar  1 2001 Bill Nottingham <notting@redhat.com>
- make netconfig much more sane (#30008, in essence)

* Tue Feb 27 2001 Erik Troan <ewt@redhat.com>
- applied patch to use SO_BINDTODEVICE properly (weejock@ferret.lmh.ox.ac.uk)

* Mon Feb 12 2001 Bill Nottingham <notting@redhat.com>
- run ifup-post on lease renewal

* Thu Feb 01 2001 Erik Troan <ewt@redhat.com>
- update secs field properly
- don't reset interface flags we don't understand
- added --win-client-id flag
- cleaned up packet creation a bit
- added --no-gateway

* Tue Jan  9 2001 Matt Wilson <msw@redhat.com>
- always set the src address of the broadcast to 0.0.0.0

* Mon Nov 20 2000 Erik Troan <ewt@redhat.com>
- lo device needs to have it's network route added 

* Fri Nov 10 2000 Bill Nottingham <notting@redhat.com>
- don't pass a random length to accept()

* Mon Oct 23 2000 Erik Troan <ewt@redhat.com>
- up scripts called for first interface information, and called even if pump
  failed

* Wed Aug 30 2000 Bernhard Rosenkraenzer <bero@redhat.com>
- Fix up the "search" entry scan so it works in Europe...

* Wed Aug 16 2000 Matt Wilson <msw@redhat.com>
- added a strerror(errno) to the "unable to set default route" syslog

* Tue Aug 15 2000 Erik Troan <ewt@redhat.com>
- reverted always put the dhcp option type as the first code in the vendor 
  field

* Mon Aug 07 2000 Erik Troan <ewt@redhat.com>
- added .net to list of top level domains
- don't crash on domains w/ no .'s in them
- syslog if adding the default route fails

* Sat Aug 05 2000 Erik Troan <ewt@redhat.com>
- net.c should use "pump.h", not <pump.h>

* Fri Aug 04 2000 Erik Troan <ewt@redhat.com>
- use BINDTODEVICE
- support nis domain names
- always put the dhcp option type as the first code in the vendor field

* Mon Jul  3 2000 Bill Nottingham <notting@redhat.com>
- add some sanity checks in dhcp.c

* Mon Jun 26 2000 Matt Wilson <msw@redhat.com>
- defattr root for devel subpackage

* Mon Jun 19 2000 Than Ngo <than@redhat.de>
- FHS fixes

* Tue Mar 28 2000 Erik Troan <ewt@redhat.com>
- added pump-devel package

* Thu Feb 24 2000 Erik Troan <ewt@redhat.com>
- set hw type properly (safford@watson.ibm.com)

* Wed Feb 23 2000 Erik Troan <ewt@redhat.com>
- fixed # parsing (aaron@schrab.com)

* Tue Feb 15 2000 Erik Troan <ewt@redhat.com>
- added script argument (Guy Delamarter <delamart@pas.rochester.edu>)
- fixed bug in hostname passing (H.J. Lu)
- fixed time displays to be in wall time, not up time (Chris Johnson)

* Wed Feb  9 2000 Bill Nottingham <notting@redhat.com>
- fix bug in netconfig - hitting 'back' causes bogus config files
  to get written

* Thu Feb 03 2000 Erik Troan <ewt@redhat.com>
- added patch from duanev@io.com which improves debug messages and
  uses /proc/uptime rather time time() -- this should be correct for
  everything but systems that are suspended during their lease time, in
  which case we'll be wrong <sigh>
- added hostname to DISCOVER and RELEASE events; hopefully this gets us
  working for all @HOME systems.
- patch from dunham@cse.msu.edu fixed /etc/resolv.conf parsing

* Wed Feb 02 2000 Cristian Gafton <gafton@redhat.com>
- fix description
- man pages are compressed

* Wed Nov 10 1999 Erik Troan <ewt@redhat.com>
- at some point a separate dhcp.c was created
- include hostname in renewal request
- changed default lease time to 6 hours
- if no hostname is specified on the command line, use gethostname()
  to request one (unless it's "localhost" or "localhost.localdomain")
- properly handle failed renewal attempts
- display (and request) syslog, lpr, ntp, font, and xdm servers

* Tue Sep 14 1999 Michael K. Johnson <johnsonm@redhat.com>
- pump processes cannot accumulate because of strange file
  descriptors (bug only showed up under rp3)

* Tue Sep  7 1999 Bill Nottingham <notting@redhat.com>
- add simple network configurator

* Wed Jun 23 1999 Erik Troan <ewt@redhat.com>
- patch from Sten Drescher for syslog debugging info 
- patch from Sten Drescher to not look past end of dhcp packet for options
- patches form Alan Cox for cleanups, malloc failures, and proper udp checksums
- handle replies with more then 3 dns servers specified
- resend dhcp_discover with proper options field
- shrank dhcp_vendor_length to 312 for rfc compliance (thanks to Ben Reed)
- added support for a config file
- don't replace search pass in /etc/resolv.conf unless we have a better one
- bringing down a device didn't work properly

* Sat May 29 1999 Erik Troan <ewt@redhat.com>
- bootp interfaces weren't being brought down properly
- segv could result if no domain name was given

* Sat May 08 1999 Erik Troan <ewt@redhat.com>
- fixed some file descriptor leakage

* Thu May 06 1999 Erik Troan <ewt@redhat.com>
- set option list so we'll work with NT
- tried to add a -h option, but I have no way of testing it :-(

* Wed Apr 28 1999 Erik Troan <ewt@redhat.com>
- closing fd 1 is important

* Mon Apr 19 1999 Bill Nottingham <notting@redhat.com>
- don't obsolete dhcpcd

* Tue Apr 06 1999 Erik Troan <ewt@redhat.com>
- retry code didn't handle failure terribly gracefully

* Tue Mar 30 1999 Erik Troan <ewt@redhat.com>
- added --lookup-hostname
- generate a DNS search path based on full domain set
- use raw socket for revieving reply; this lets us work properly on 2.2
  kernels when we recieve unicast replies from the bootp server

* Mon Mar 22 1999 Erik Troan <ewt@redhat.com>
- it was always requesting a 20 second lease

* Mon Mar 22 1999 Michael K. Johnson <johnsonm@redhat.com>
- added minimal man page /usr/man/man8/pump.8
