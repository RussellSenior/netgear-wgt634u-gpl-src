#! /bin/sh

## file to correct chown/chmod for package ppp
# generated 18.11.2002 02:44

# set PREFIX here
PREFIX=/home/changcs/usb-router-buildroot/build/packages/pptp

# use commandline to overwrite prefix

 if  [ "x$1" != "x" ] ; then 
PREFIX=/home/changcs/usb-router-buildroot/build/packages/pptp
fi


# just to be sure 
if [ "x$PREFIX" = "x" ] ; then
echo no prexix set, this is DANGEROUS, exiting
 exit 1
fi

# file: etc/
chown root.root ${PREFIX}/etc/
chmod u=rwx,g=rx,o=rx ${PREFIX}/etc/

# file: etc/ppp/
chown root.root ${PREFIX}/etc/ppp/
chmod u=rwx,g=rx,o=rx ${PREFIX}/etc/ppp/

# file: etc/ppp/peers
chown root.root ${PREFIX}/etc/ppp/peers/
chmod u=rwx,g=rx,o= ${PREFIX}/etc/ppp/peers/

# file: etc/ppp/peers/pptp-provider
chown root.root ${PREFIX}/etc/ppp/peers/pptp-provider
chmod u=rw,g=r,o= ${PREFIX}/etc/ppp/peers/pptp-provider

# file: usr/
chown root.root ${PREFIX}/usr/
chmod u=rwx,g=rx,o=rx ${PREFIX}/usr/

# file: usr/sbin/
chown root.root ${PREFIX}/usr/sbin/
chmod u=rwx,g=rx,o=rx ${PREFIX}/usr/sbin/

# file: usr/sbin/pptp
chown root.root ${PREFIX}/usr/sbin/pptp
chmod u=rwx,g=rx,o=rx ${PREFIX}/usr/sbin/pptp

# file: var/
chown root.root ${PREFIX}/var/
chmod u=rwx,g=rx,o=rx ${PREFIX}/var/

# file: var/lib/
chown root.root ${PREFIX}/var/lib/
chmod u=rwx,g=rx,o=rx ${PREFIX}/var/lib/

# file: var/lib/lrpkg/
chown root.root ${PREFIX}/var/lib/lrpkg/
chmod u=rwx,g=rx,o=rx ${PREFIX}/var/lib/lrpkg/

# file: var/lib/lrpkg/ppp.version
chown root.root ${PREFIX}/var/lib/lrpkg/pptp.version
chmod u=rw,g=r,o=r ${PREFIX}/var/lib/lrpkg/pptp.version

# file: var/lib/lrpkg/ppp.help
chown root.root ${PREFIX}/var/lib/lrpkg/pptp.help
chmod u=rw,g=r,o=r ${PREFIX}/var/lib/lrpkg/pptp.help

# file: var/lib/lrpkg/ppp.list
chown root.root ${PREFIX}/var/lib/lrpkg/pptp.list
chmod u=rw ${PREFIX}/var/lib/lrpkg/pptp.list

# file: var/lib/lrpkg/ppp.conf
chown root.root ${PREFIX}/var/lib/lrpkg/pptp.conf
chmod u=rw ${PREFIX}/var/lib/lrpkg/pptp.conf
