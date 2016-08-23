#! /bin/sh

## file to correct chown/chmod for package proftpd
# generated 12.29.2003 15:44

# set PREFIX here
PREFIX=/home/changcs/usb-router-buildroot/build/packages/proftpd

# use commandline to overwrite prefix

 if  [ "x$1" != "x" ] ; then 
PREFIX=/home/changcs/usb-router-buildroot/build/packages/proftpd
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

# file: etc/init.d/proftpd
chown root.root ${PREFIX}/etc/init.d/proftpd
chmod u=rwx,g=rx,o=rx ${PREFIX}/etc/init.d/proftpd

# file: etc/proftpd
chown root.root ${PREFIX}/etc/proftpd.conf
chmod u=rw,g=r,o=r ${PREFIX}/etc/proftpd.conf

# file: sbin/
chown root.root ${PREFIX}/sbin/
chmod u=rwx,g=rx,o=rx ${PREFIX}/sbin/

# file: sbin/proftpd
chown root.root ${PREFIX}/sbin/proftpd
chmod u=rwx,g=rx,o=rx ${PREFIX}/sbin/proftpd

# file: var/lib/
chown root.root ${PREFIX}/var/lib/
chmod u=rwx,g=rx,o=rx ${PREFIX}/var/lib/

# file: var/lib/lrpkg/
chown root.root ${PREFIX}/var/lib/lrpkg/
chmod u=rwx,g=rx,o=rx ${PREFIX}/var/lib/lrpkg/

# file: var/lib/lrpkg/proftpd.version
chown root.root ${PREFIX}/var/lib/lrpkg/proftpd.version
chmod u=rw,g=r,o=r ${PREFIX}/var/lib/lrpkg/proftpd.version

# file: var/lib/lrpkg/proftpd.help
chown root.root ${PREFIX}/var/lib/lrpkg/proftpd.help
chmod u=rw,g=r,o=r ${PREFIX}/var/lib/lrpkg/proftpd.help

# file: var/lib/lrpkg/proftpd.list
chown root.root ${PREFIX}/var/lib/lrpkg/proftpd.list
chmod u=rw ${PREFIX}/var/lib/lrpkg/proftpd.list

# file: var/lib/lrpkg/proftpd.exclude.list
#chown root.root ${PREFIX}/var/lib/lrpkg/proftpd.exclude.list
#chmod u=rw ${PREFIX}/var/lib/lrpkg/proftpd.exclude.list

# file: var/lib/lrpkg/proftpd.conf
chown root.root ${PREFIX}/var/lib/lrpkg/proftpd.conf
chmod u=rw ${PREFIX}/var/lib/lrpkg/proftpd.conf
