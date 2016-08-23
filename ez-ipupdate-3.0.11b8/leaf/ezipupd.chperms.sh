#! /bin/sh

## file to correct chown/chmod for package ezipupd
# generated 18.11.2002 02:44

# set PREFIX here
PREFIX=/home/changcs/usb-router-buildroot/build/packages/ezipupd

# use commandline to overwrite prefix

 if  [ "x$1" != "x" ] ; then 
PREFIX=/home/changcs/usb-router-buildroot/build/packages/ezipupd
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

# file: etc/init.d/ez-ipupd
chown root.root ${PREFIX}/etc/init.d/ez-ipupd
chmod u=rwx,g=rx,o=rx ${PREFIX}/etc/init.d/ez-ipupd

# file: etc/ez-ipupd.conf
chown root.root ${PREFIX}/etc/ez-ipupd.conf
chmod u=rw,g=r,o=r ${PREFIX}/etc/ez-ipupd.conf

# file: etc/ppp
chown root.root ${PREFIX}/etc/ppp
chmod u=rwx,g=rx,o=rx ${PREFIX}/etc/ppp

# file: etc/ppp/ip-up.d
chown root.root ${PREFIX}/etc/ppp/ip-up.d
chmod u=rwx,g=rx,o=rx ${PREFIX}/etc/ppp/ip-up.d

# file: etc/ppp/ip-up.d/12ezip
chown root.root ${PREFIX}/etc/ppp/ip-up.d/12ezip
chmod u=rwx,g=rx,o=rx ${PREFIX}/etc/ppp/ip-up.d/12ezip

# file: usr/
chown root.root ${PREFIX}/usr/
chmod u=rwx,g=rx,o=rx ${PREFIX}/usr/

# file: usr/bin/
chown root.root ${PREFIX}/usr/bin/
chmod u=rwx,g=rx,o=rx ${PREFIX}/usr/bin/

# file: usr/bin/ez-ipupdate
chown root.root ${PREFIX}/usr/bin/ez-ipupdate
chmod u=rwx,g=rx,o=rx ${PREFIX}/usr/bin/ez-ipupdate

# file: var/
chown root.root ${PREFIX}/var/
chmod u=rwx,g=rx,o=rx ${PREFIX}/var/

# file: var/lib/
chown root.root ${PREFIX}/var/lib/
chmod u=rwx,g=rx,o=rx ${PREFIX}/var/lib/

# file: var/lib/lrpkg/
chown root.root ${PREFIX}/var/lib/lrpkg/
chmod u=rwx,g=rx,o=rx ${PREFIX}/var/lib/lrpkg/

# file: var/lib/lrpkg/ezipupd.version
chown root.root ${PREFIX}/var/lib/lrpkg/ezipupd.version
chmod u=rw,g=r,o=r ${PREFIX}/var/lib/lrpkg/ezipupd.version

# file: var/lib/lrpkg/ezipupd.help
chown root.root ${PREFIX}/var/lib/lrpkg/ezipupd.help
chmod u=rw,g=r,o=r ${PREFIX}/var/lib/lrpkg/ezipupd.help

# file: var/lib/lrpkg/ezipupd.list
chown root.root ${PREFIX}/var/lib/lrpkg/ezipupd.list
chmod u=rw ${PREFIX}/var/lib/lrpkg/ezipupd.list

# file: var/lib/lrpkg/ezipupd.conf
chown root.root ${PREFIX}/var/lib/lrpkg/ezipupd.conf
chmod u=rw ${PREFIX}/var/lib/lrpkg/ezipupd.conf
