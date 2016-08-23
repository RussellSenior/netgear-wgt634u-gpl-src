#! /bin/sh

## file to correct chown/chmod for package zebra
# generated 18.11.2002 02:44

# set PREFIX here
PREFIX=/home/changcs/usb-router-buildroot/build/packages/zebra

# use commandline to overwrite prefix

 if  [ "x$1" != "x" ] ; then 
PREFIX=/home/changcs/usb-router-buildroot/build/packages/zebra
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

# file: etc/init.d/zebra
chown root.root ${PREFIX}/etc/init.d/zebra
chmod u=rwx,g=rx,o=rx ${PREFIX}/etc/init.d/zebra

# file: etc/init.d/ripd
chown root.root ${PREFIX}/etc/init.d/ripd
chmod u=rwx,g=rx,o=rx ${PREFIX}/etc/init.d/ripd

# file: etc/zebra
chown root.root ${PREFIX}/etc/zebra
chmod u=rwx,g=rx,o=rx ${PREFIX}/etc/zebra

# file: etc/zebra/zebra.conf
chown root.root ${PREFIX}/etc/zebra/zebra.conf
chmod u=rw,g=r,o=r ${PREFIX}/etc/zebra/zebra.conf

# file: etc/zebra/ripd.conf
chown root.root ${PREFIX}/etc/zebra/ripd.conf
chmod u=rw,g=r,o=r ${PREFIX}/etc/zebra/ripd.conf

# file: usr/
chown root.root ${PREFIX}/usr/
chmod u=rwx,g=rx,o=rx ${PREFIX}/usr/

# file: usr/sbin/
chown root.root ${PREFIX}/usr/sbin/
chmod u=rwx,g=rx,o=rx ${PREFIX}/usr/sbin/

# file: usr/sbin/zebra
chown root.root ${PREFIX}/usr/sbin/zebra
chmod u=rwx,g=rx,o=rx ${PREFIX}/usr/sbin/zebra

# file: usr/sbin/ripd
chown root.root ${PREFIX}/usr/sbin/ripd
chmod u=rwx,g=rx,o=rx ${PREFIX}/usr/sbin/ripd

# file: var/
chown root.root ${PREFIX}/var/
chmod u=rwx,g=rx,o=rx ${PREFIX}/var/

# file: var/lib/
chown root.root ${PREFIX}/var/lib/
chmod u=rwx,g=rx,o=rx ${PREFIX}/var/lib/

# file: var/lib/lrpkg/
chown root.root ${PREFIX}/var/lib/lrpkg/
chmod u=rwx,g=rx,o=rx ${PREFIX}/var/lib/lrpkg/

# file: var/lib/lrpkg/zebra.version
chown root.root ${PREFIX}/var/lib/lrpkg/zebra.version
chmod u=rw,g=r,o=r ${PREFIX}/var/lib/lrpkg/zebra.version

# file: var/lib/lrpkg/zebra.help
chown root.root ${PREFIX}/var/lib/lrpkg/zebra.help
chmod u=rw,g=r,o=r ${PREFIX}/var/lib/lrpkg/zebra.help

# file: var/lib/lrpkg/zebra.list
chown root.root ${PREFIX}/var/lib/lrpkg/zebra.list
chmod u=rw ${PREFIX}/var/lib/lrpkg/zebra.list

# file: var/lib/lrpkg/zebra.conf
chown root.root ${PREFIX}/var/lib/lrpkg/zebra.conf
chmod u=rw ${PREFIX}/var/lib/lrpkg/zebra.conf
