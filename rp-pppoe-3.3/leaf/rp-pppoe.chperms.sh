#! /bin/sh

## file to correct chown/chmod for package rp-pppoe 
# generated 12.29.2003 15:44

# set PREFIX here
PREFIX=/home/changcs/usb-router-buildroot/build/packages/rp-pppoe

# use commandline to overwrite prefix

 if  [ "x$1" != "x" ] ; then 
PREFIX=/home/changcs/usb-router-buildroot/build/packages/rp-pppoe
fi


# just to be sure 
if [ "x$PREFIX" = "x" ] ; then
echo no prexix set, this is DANGEROUS, exiting
 exit 1
fi

# file: usr/
chown root.root ${PREFIX}/usr/
chmod u=rwx,g=rx,o=rx ${PREFIX}/usr/

# file: usr/sbin
chown root.root ${PREFIX}/usr/sbin
chmod u=rwx,g=rx,o=rx ${PREFIX}/usr/sbin

# file: usr/sbin/pppoe
chown root.root ${PREFIX}/usr/sbin/pppoe
chmod u=rwx,g=rx,o=rx ${PREFIX}/usr/sbin/pppoe

# file: var/
chown root.root ${PREFIX}/var/
chmod u=rwx,g=rx,o=rx ${PREFIX}/var/

# file: var/lib/
chown root.root ${PREFIX}/var/lib/
chmod u=rwx,g=rx,o=rx ${PREFIX}/var/lib/

# file: var/lib/lrpkg/
chown root.root ${PREFIX}/var/lib/lrpkg/
chmod u=rwx,g=rx,o=rx ${PREFIX}/var/lib/lrpkg/

# file: var/lib/lrpkg/rp-pppoe.version
chown root.root ${PREFIX}/var/lib/lrpkg/rp-pppoe.version
chmod u=rw,g=r,o=r ${PREFIX}/var/lib/lrpkg/rp-pppoe.version

# file: var/lib/lrpkg/rp-pppoe.help
chown root.root ${PREFIX}/var/lib/lrpkg/rp-pppoe.help
chmod u=rw,g=r,o=r ${PREFIX}/var/lib/lrpkg/rp-pppoe.help

# file: var/lib/lrpkg/rp-pppoe.list
chown root.root ${PREFIX}/var/lib/lrpkg/rp-pppoe.list
chmod u=rw ${PREFIX}/var/lib/lrpkg/rp-pppoe.list

# file: var/lib/lrpkg/rp-pppoe.exclude.list
#chown root.root ${PREFIX}/var/lib/lrpkg/rp-pppoe.exclude.list
#chmod u=rw ${PREFIX}/var/lib/lrpkg/rp-pppoe.exclude.list

# file: var/lib/lrpkg/rp-pppoe.conf
chown root.root ${PREFIX}/var/lib/lrpkg/rp-pppoe.conf
chmod u=rw ${PREFIX}/var/lib/lrpkg/rp-pppoe.conf
