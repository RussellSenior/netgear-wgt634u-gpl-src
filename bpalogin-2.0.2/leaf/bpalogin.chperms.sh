#! /bin/sh

## file to correct chown/chmod for package bpalogin
# generated 12.29.2003 15:44

# set PREFIX here
PREFIX=/home/changcs/usb-router-buildroot/build/packages/bpalogin

# use commandline to overwrite prefix

 if  [ "x$1" != "x" ] ; then 
PREFIX=/home/changcs/usb-router-buildroot/build/packages/bpalogin
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

# file: etc/init.d/bpalogin
chown root.root ${PREFIX}/etc/init.d/bpalogin
chmod u=rwx,g=rx,o=rx ${PREFIX}/etc/init.d/bpalogin

# file: etc/bpalogin.conf
chown root.root ${PREFIX}/etc/bpalogin.conf
chmod u=rw,g=r,o=r ${PREFIX}/etc/bpalogin.conf

# file: /usr/
chown root.root ${PREFIX}/usr/
chmod u=rwx,g=rx,o=rx ${PREFIX}/usr/

# file: /usr/sbin/
chown root.root ${PREFIX}/usr/sbin/
chmod u=rwx,g=rx,o=rx ${PREFIX}/usr/sbin/

# file: /usr/sbin/bpalogin
chown root.root ${PREFIX}/usr/sbin/bpalogin
chmod u=rwx,g=rx,o=rx ${PREFIX}/usr/sbin/bpalogin

# file: var/lib/
chown root.root ${PREFIX}/var/lib/
chmod u=rwx,g=rx,o=rx ${PREFIX}/var/lib/

# file: var/lib/lrpkg/
chown root.root ${PREFIX}/var/lib/lrpkg/
chmod u=rwx,g=rx,o=rx ${PREFIX}/var/lib/lrpkg/

# file: var/lib/lrpkg/bpalogin.version
chown root.root ${PREFIX}/var/lib/lrpkg/bpalogin.version
chmod u=rw,g=r,o=r ${PREFIX}/var/lib/lrpkg/bpalogin.version

# file: var/lib/lrpkg/bpalogin.help
chown root.root ${PREFIX}/var/lib/lrpkg/bpalogin.help
chmod u=rw,g=r,o=r ${PREFIX}/var/lib/lrpkg/bpalogin.help

# file: var/lib/lrpkg/bpalogin.list
chown root.root ${PREFIX}/var/lib/lrpkg/bpalogin.list
chmod u=rw ${PREFIX}/var/lib/lrpkg/bpalogin.list

# file: var/lib/lrpkg/proftpd.exclude.list
#chown root.root ${PREFIX}/var/lib/lrpkg/proftpd.exclude.list
#chmod u=rw ${PREFIX}/var/lib/lrpkg/proftpd.exclude.list
