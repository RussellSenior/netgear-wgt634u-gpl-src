#! /bin/sh

## file to correct chown/chmod for package dnsmasq
# generated 18.11.2002 02:44

# set PREFIX here
PREFIX=/home/changcs/usb-router-buildroot/build/packages/dnsmasq

# use commandline to overwrite prefix

 if  [ "x$1" != "x" ] ; then 
PREFIX=/home/changcs/usb-router-buildroot/build/packages/dnsmasq
fi


# just to be sure 
if [ "x$PREFIX" = "x" ] ; then
echo no prexix set, this is DANGEROUS, exiting
 exit 1
fi

# file: etc/
chown root.root ${PREFIX}/etc/
chmod u=rwx,g=rx,o=rx ${PREFIX}/etc/

# file: etc/resolv.static1
chown root.root ${PREFIX}/etc/resolv.static1
chmod u=rw,g=r,o=r ${PREFIX}/etc/resolv.static1

# file: etc/resolv.static2
chown root.root ${PREFIX}/etc/resolv.static2
chmod u=rw,g=r,o=r ${PREFIX}/etc/resolv.static2

# file: etc/init.d/
chown root.root ${PREFIX}/etc/init.d/
chmod u=rwx,g=rx,o=rx ${PREFIX}/etc/init.d/

# file: etc/init.d/dnsmisc.sh
chown root.root ${PREFIX}/etc/init.d/dnsmisc.sh
chmod u=rwx,g=rx,o=rx ${PREFIX}/etc/init.d/dnsmisc.sh

# file: etc/init.d/dnsmasq
chown root.root ${PREFIX}/etc/init.d/dnsmasq
chmod u=rwx,g=rx,o=rx ${PREFIX}/etc/init.d/dnsmasq

# file: etc/dnsmasq.conf
chown root.root ${PREFIX}/etc/dnsmasq.conf
chmod u=rw,g=r,o=r ${PREFIX}/etc/dnsmasq.conf

# file: usr/
chown root.root ${PREFIX}/usr/
chmod u=rwx,g=rx,o=rx ${PREFIX}/usr/

# file: usr/sbin/
chown root.root ${PREFIX}/usr/sbin/
chmod u=rwx,g=rx,o=rx ${PREFIX}/usr/sbin/

# file: usr/sbin/dnsmasq
chown root.root ${PREFIX}/usr/sbin/dnsmasq
chmod u=rwx,g=rx,o=rx ${PREFIX}/usr/sbin/dnsmasq

# file: var/
chown root.root ${PREFIX}/var/
chmod u=rwx,g=rx,o=rx ${PREFIX}/var/

# file: var/lib/
chown root.root ${PREFIX}/var/lib/
chmod u=rwx,g=rx,o=rx ${PREFIX}/var/lib/

# file: var/lib/lrpkg/
chown root.root ${PREFIX}/var/lib/lrpkg/
chmod u=rwx,g=rx,o=rx ${PREFIX}/var/lib/lrpkg/

# file: var/lib/lrpkg/dnsmasq.version
chown root.root ${PREFIX}/var/lib/lrpkg/dnsmasq.version
chmod u=rw,g=r,o=r ${PREFIX}/var/lib/lrpkg/dnsmasq.version

# file: var/lib/lrpkg/dnsmasq.help
chown root.root ${PREFIX}/var/lib/lrpkg/dnsmasq.help
chmod u=rw,g=r,o=r ${PREFIX}/var/lib/lrpkg/dnsmasq.help

# file: var/lib/lrpkg/dnsmasq.list
chown root.root ${PREFIX}/var/lib/lrpkg/dnsmasq.list
chmod u=rw ${PREFIX}/var/lib/lrpkg/dnsmasq.list

# file: var/lib/lrpkg/dnsmasq.conf
chown root.root ${PREFIX}/var/lib/lrpkg/dnsmasq.conf
chmod u=rw ${PREFIX}/var/lib/lrpkg/dnsmasq.conf
