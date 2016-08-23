#! /bin/sh

## file to correct chown/chmod for package boa
# generated 12.29.2003 15:44

# set PREFIX here
PREFIX=/home/changcs/usb-router-buildroot/build/packages/boa

# use commandline to overwrite prefix

 if  [ "x$1" != "x" ] ; then 
PREFIX=/home/changcs/usb-router-buildroot/build/packages/boa
fi


# just to be sure 
if [ "x$PREFIX" = "x" ] ; then
echo no prexix set, this is DANGEROUS, exiting
 exit 1
fi

# file: etc/
chown root.root ${PREFIX}/etc/
chmod u=rwx,g=rx,o=rx ${PREFIX}/etc/

# file: etc/init.d
chown root.root ${PREFIX}/etc/init.d
chmod u=rwx,g=rx,o=rx ${PREFIX}/etc/init.d

# file: etc/init.d/boa
chown root.root ${PREFIX}/etc/init.d/boa
chmod u=rwx,g=rx,o=rx ${PREFIX}/etc/init.d/boa

# file: etc/boa
chown root.root ${PREFIX}/etc/boa
chmod u=rwx,g=rx,o=rx ${PREFIX}/etc/boa

# file: etc/boa/boa.conf
chown root.root ${PREFIX}/etc/boa/boa.conf
chmod u=rw,g=r,o=r ${PREFIX}/etc/boa/boa.conf

# file: etc/mime.types
chown root.root ${PREFIX}/etc/mime.types
chmod u=rw,g=r,o=r ${PREFIX}/etc/mime.types

# file: usr/
chown root.root ${PREFIX}/usr/
chmod u=rwx,g=rx,o=rx ${PREFIX}/usr/

# file: usr/sbin/
chown root.root ${PREFIX}/usr/sbin/
chmod u=rwx,g=rx,o=rx ${PREFIX}/usr/sbin/

# file: usr/sbin/boa
chown root.root ${PREFIX}/usr/sbin/boa
chmod u=rwx,g=rx,o=rx ${PREFIX}/usr/sbin/boa

# file: usr/lib/
chown root.root ${PREFIX}/usr/lib/
chmod u=rwx,g=rx,o=rx ${PREFIX}/usr/lib/

# file: usr/lib/boa/
chown root.root ${PREFIX}/usr/lib/boa/
chmod u=rwx,g=rx,o=rx ${PREFIX}/usr/lib/boa/

# file: usr/lib/boa/boa_indexer
chown root.root ${PREFIX}/usr/lib/boa/boa_indexer
chmod u=rwx,g=rx,o=rx ${PREFIX}/usr/lib/boa/boa_indexer

# file: usr/lib/cgi-bin/
chown root.root ${PREFIX}/usr/lib/cgi-bin/
chmod u=rwx,g=rx,o=rx ${PREFIX}/usr/lib/cgi-bin/

# file: usr/lib/cgi-bin/proccgi
chown root.root ${PREFIX}/usr/lib/cgi-bin/proccgi
chmod u=rwx,g=rx,o=rx ${PREFIX}/usr/lib/cgi-bin/proccgi

# file: var/
chown root.root ${PREFIX}/var/
chmod u=rwx,g=rx,o=rx ${PREFIX}/var/

# file: var/lib/
chown root.root ${PREFIX}/var/lib/
chmod u=rwx,g=rx,o=rx ${PREFIX}/var/lib/

# file: var/lib/lrpkg/
chown root.root ${PREFIX}/var/lib/lrpkg/
chmod u=rwx,g=rx,o=rx ${PREFIX}/var/lib/lrpkg/

# file: var/lib/lrpkg/boa.version
chown root.root ${PREFIX}/var/lib/lrpkg/boa.version
chmod u=rw,g=r,o=r ${PREFIX}/var/lib/lrpkg/boa.version

# file: var/lib/lrpkg/boa.help
chown root.root ${PREFIX}/var/lib/lrpkg/boa.help
chmod u=rw,g=r,o=r ${PREFIX}/var/lib/lrpkg/boa.help

# file: var/lib/lrpkg/boa.list
chown root.root ${PREFIX}/var/lib/lrpkg/boa.list
chmod u=rw ${PREFIX}/var/lib/lrpkg/boa.list

# file: var/lib/lrpkg/boa.conf
chown root.root ${PREFIX}/var/lib/lrpkg/boa.conf
chmod u=rw ${PREFIX}/var/lib/lrpkg/boa.conf
