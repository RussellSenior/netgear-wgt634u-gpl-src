#! /bin/sh

## file to correct chown/chmod for package dhcpcd
# generated 18.11.2002 02:44

# set PREFIX here
PREFIX=/home/changcs/usb-router-buildroot/build/packages/dhcpcd

# use commandline to overwrite prefix

 if  [ "x$1" != "x" ] ; then 
PREFIX=/home/changcs/usb-router-buildroot/build/packages/dhcpcd
fi


# just to be sure 
if [ "x$PREFIX" = "x" ] ; then
echo no prexix set, this is DANGEROUS, exiting
 exit 1
fi

# file: etc/
chown root.root ${PREFIX}/etc/
chmod u=rwx,g=rx,o=rx ${PREFIX}/etc/

# file: etc/dhcpc/
chown root.root ${PREFIX}/etc/dhcpc/
chmod u=rwx,g=rx,o=rx ${PREFIX}/etc/dhcpc/

# file: etc/dhcpc/config
chown root.root ${PREFIX}/etc/dhcpc/config
chmod u=rw,g=r,o=r ${PREFIX}/etc/dhcpc/config

# file: etc/dhcpc/dhcpcd.exe
chown root.root ${PREFIX}/etc/dhcpc/dhcpcd.exe
chmod u=rwx,g=rx,o=rx ${PREFIX}/etc/dhcpc/dhcpcd.exe

# file: sbin/
chown root.root ${PREFIX}/sbin/
chmod u=rwx,g=rx,o=rx ${PREFIX}/sbin/

# file: sbin/dhcpcd
chown root.root ${PREFIX}/sbin/dhcpcd
chmod u=rwx,g=rx,o=rx ${PREFIX}/sbin/dhcpcd

# file: sbin/dhcpcd-bin
chown root.root ${PREFIX}/sbin/dhcpcd-bin
chmod u=rwx,g=rx,o=rx ${PREFIX}/sbin/dhcpcd-bin

# file: var/
chown root.root ${PREFIX}/var/
chmod u=rwx,g=rx,o=rx ${PREFIX}/var/

# file: var/lib/
chown root.root ${PREFIX}/var/lib/
chmod u=rwx,g=rx,o=rx ${PREFIX}/var/lib/

# file: var/lib/lrpkg/
chown root.root ${PREFIX}/var/lib/lrpkg/
chmod u=rwx,g=rx,o=rx ${PREFIX}/var/lib/lrpkg/

# file: var/lib/lrpkg/dhcpcd.version
chown root.root ${PREFIX}/var/lib/lrpkg/dhcpcd.version
chmod u=rw,g=r,o=r ${PREFIX}/var/lib/lrpkg/dhcpcd.version

# file: var/lib/lrpkg/dhcpcd.help
chown root.root ${PREFIX}/var/lib/lrpkg/dhcpcd.help
chmod u=rw,g=r,o=r ${PREFIX}/var/lib/lrpkg/dhcpcd.help

# file: var/lib/lrpkg/dhcpcd.list
chown root.root ${PREFIX}/var/lib/lrpkg/dhcpcd.list
chmod u=rw ${PREFIX}/var/lib/lrpkg/dhcpcd.list

# file: var/lib/lrpkg/dhcpcd.conf
chown root.root ${PREFIX}/var/lib/lrpkg/dhcpcd.conf
chmod u=rw ${PREFIX}/var/lib/lrpkg/dhcpcd.conf

# file: var/lib/lrpkg/dhcpcd.exclude.list
chown root.root ${PREFIX}/var/lib/lrpkg/dhcpcd.exclude.list
chmod u=rw ${PREFIX}/var/lib/lrpkg/dhcpcd.exclude.list

