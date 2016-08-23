#! /bin/sh

## file to correct chown/chmod for package uupnpd
# generated 18.11.2002 02:44

# set PREFIX here
PREFIX=/home/changcs/usb-router-buildroot/build/packages/linuxigd

# use commandline to overwrite prefix

 if  [ "x$1" != "x" ] ; then 
PREFIX=/home/changcs/usb-router-buildroot/build/packages/linuxigd
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

# file: etc/init.d/upnpd
chown root.root ${PREFIX}/etc/init.d/upnpd
chmod u=rwx,g=rx,o=rx ${PREFIX}/etc/init.d/upnpd

# file: usr/
chown root.root ${PREFIX}/usr/
chmod u=rwx,g=rx,o=rx ${PREFIX}/usr/

# file: usr/sbin/
chown root.root ${PREFIX}/usr/sbin/
chmod u=rwx,g=rx,o=rx ${PREFIX}/usr/sbin/

# file: usr/sbin/upnpd
chown root.root ${PREFIX}/usr/sbin/upnpd
chmod u=rwx,g=rx,o=rx ${PREFIX}/usr/sbin/upnpd

# file: var/
chown root.root ${PREFIX}/var/
chmod u=rwx,g=rx,o=rx ${PREFIX}/var/

# file: var/lib/
chown root.root ${PREFIX}/var/lib/
chmod u=rwx,g=rx,o=rx ${PREFIX}/var/lib/

# file: var/lib/lrpkg/
chown root.root ${PREFIX}/var/lib/lrpkg/
chmod u=rwx,g=rx,o=rx ${PREFIX}/var/lib/lrpkg/

# file: var/lib/lrpkg/upnpd.version
chown root.root ${PREFIX}/var/lib/lrpkg/upnpd.version
chmod u=rw,g=r,o=r ${PREFIX}/var/lib/lrpkg/upnpd.version

# file: var/lib/lrpkg/upnpd.help
chown root.root ${PREFIX}/var/lib/lrpkg/upnpd.help
chmod u=rw,g=r,o=r ${PREFIX}/var/lib/lrpkg/upnpd.help

# file: var/lib/lrpkg/upnpd.list
chown root.root ${PREFIX}/var/lib/lrpkg/upnpd.list
chmod u=rw ${PREFIX}/var/lib/lrpkg/upnpd.list

# file: var/lib/lrpkg/upnpd.conf
chown root.root ${PREFIX}/var/lib/lrpkg/upnpd.conf
chmod u=rw ${PREFIX}/var/lib/lrpkg/upnpd.conf

