#! /bin/sh

## file to correct chown/chmod for package iptables
# generated 18.11.2002 02:44

# set PREFIX here
PREFIX=/home/changcs/usb-router-buildroot/build/packages/iptables

# use commandline to overwrite prefix

 if  [ "x$1" != "x" ] ; then 
PREFIX=/home/changcs/usb-router-buildroot/build/packages/iptables
fi


# just to be sure 
if [ "x$PREFIX" = "x" ] ; then
echo no prexix set, this is DANGEROUS, exiting
 exit 1
fi

# file: sbin/
chown root.root ${PREFIX}/sbin/
chmod u=rwx,g=rx,o=rx ${PREFIX}/sbin/

# file: sbin/iptables
chown root.root ${PREFIX}/sbin/iptables
chmod u=rwx,g=rx,o=rx ${PREFIX}/sbin/iptables

# file: var/
chown root.root ${PREFIX}/var/
chmod u=rwx,g=rx,o=rx ${PREFIX}/var/

# file: var/lib/
chown root.root ${PREFIX}/var/lib/
chmod u=rwx,g=rx,o=rx ${PREFIX}/var/lib/

# file: var/lib/lrpkg/
chown root.root ${PREFIX}/var/lib/lrpkg/
chmod u=rwx,g=rx,o=rx ${PREFIX}/var/lib/lrpkg/

# file: var/lib/lrpkg/iptables.version
chown root.root ${PREFIX}/var/lib/lrpkg/iptables.version
chmod u=rw,g=r,o=r ${PREFIX}/var/lib/lrpkg/iptables.version

# file: var/lib/lrpkg/iptables.help
chown root.root ${PREFIX}/var/lib/lrpkg/iptables.help
chmod u=rw,g=r,o=r ${PREFIX}/var/lib/lrpkg/iptables.help

# file: var/lib/lrpkg/iptables.list
chown root.root ${PREFIX}/var/lib/lrpkg/iptables.list
chmod u=rw ${PREFIX}/var/lib/lrpkg/iptables.list
