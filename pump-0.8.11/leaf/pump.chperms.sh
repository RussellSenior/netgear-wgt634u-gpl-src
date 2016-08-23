#! /bin/sh

## file to correct chown/chmod for package pump
# generated 18.11.2002 02:44

# set PREFIX here
PREFIX=/home/changcs/usb-router-buildroot/build/packages/pump

# use commandline to overwrite prefix

 if  [ "x$1" != "x" ] ; then 
PREFIX=/home/changcs/usb-router-buildroot/build/packages/pump
fi


# just to be sure 
if [ "x$PREFIX" = "x" ] ; then
echo no prexix set, this is DANGEROUS, exiting
 exit 1
fi

# file: etc/
chown root.root ${PREFIX}/etc/
chmod u=rwx,g=rx,o=rx ${PREFIX}/etc/

# file: etc/pump.conf
chown root.root ${PREFIX}/etc/pump.conf
chmod u=rw,g=r,o=r ${PREFIX}/etc/pump.conf

# file: etc/pump.shorewall
chown root.root ${PREFIX}/etc/pump.shorewall
chmod u=rwx,g=rx,o=rx ${PREFIX}/etc/pump.shorewall

# file: sbin/
chown root.root ${PREFIX}/sbin/
chmod u=rwx,g=rx,o=rx ${PREFIX}/sbin/

# file: sbin/pump
chown root.root ${PREFIX}/sbin/
chmod u=rwx,g=rx,o=rx ${PREFIX}/sbin/pump

# file: var/
chown root.root ${PREFIX}/var/
chmod u=rwx,g=rx,o=rx ${PREFIX}/var/

# file: var/lib/
chown root.root ${PREFIX}/var/lib/
chmod u=rwx,g=rx,o=rx ${PREFIX}/var/lib/

# file: var/lib/lrpkg/
chown root.root ${PREFIX}/var/lib/lrpkg/
chmod u=rwx,g=rx,o=rx ${PREFIX}/var/lib/lrpkg/

# file: var/lib/lrpkg/pump.version
chown root.root ${PREFIX}/var/lib/lrpkg/pump.version
chmod u=rw,g=r,o=r ${PREFIX}/var/lib/lrpkg/pump.version

# file: var/lib/lrpkg/pump.help
chown root.root ${PREFIX}/var/lib/lrpkg/pump.help
chmod u=rw,g=r,o=r ${PREFIX}/var/lib/lrpkg/pump.help

# file: var/lib/lrpkg/pump.list
chown root.root ${PREFIX}/var/lib/lrpkg/pump.list
chmod u=rw ${PREFIX}/var/lib/lrpkg/pump.list

# file: var/lib/lrpkg/pump.conf
chown root.root ${PREFIX}/var/lib/lrpkg/pump.conf
chmod u=rw ${PREFIX}/var/lib/lrpkg/pump.conf
