#! /bin/sh

## file to correct chown/chmod for package boa
# generated 12.29.2003 15:44

# set PREFIX here
PREFIX=/home/changcs/usb-router-buildroot/build/packages/samba

# use commandline to overwrite prefix

 if  [ "x$1" != "x" ] ; then 
PREFIX=/home/changcs/usb-router-buildroot/build/packages/samba
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

# file: etc/init.d/samba
chown root.root ${PREFIX}/etc/init.d/samba
chmod u=rwx,g=rx,o=rx ${PREFIX}/etc/init.d/samba

# file: etc/samba
chown root.root ${PREFIX}/etc/samba
chmod u=rwx,g=rx,o=rx ${PREFIX}/etc/samba

# file: etc/samaba/smb.conf
chown root.root ${PREFIX}/etc/samba/smb.conf
chmod u=rw,g=r,o=r ${PREFIX}/etc/samba/smb.conf

# file: sbin/
chown root.root ${PREFIX}/sbin/
chmod u=rwx,g=rx,o=rx ${PREFIX}/sbin/

# file: sbin/smbd
chown root.root ${PREFIX}/sbin/smbd
chmod u=rwx,g=rx,o=rx ${PREFIX}/sbin/smbd

# file: sbin/nmbd
chown root.root ${PREFIX}/sbin/nmbd
chmod u=rwx,g=rx,o=rx ${PREFIX}/sbin/nmbd

# file: var/
chown root.root ${PREFIX}/var/
chmod u=rwx,g=rx,o=rx ${PREFIX}/var/

# file: var/locks/
chown root.root ${PREFIX}/var/locks/
chmod u=rwx,g=rx,o=rx ${PREFIX}/var/locks/

# file: var/locks/.touch
chown root.root ${PREFIX}/var/locks/.touch
chmod u=rw ${PREFIX}/var/locks/.touch

# file: var/lib/
chown root.root ${PREFIX}/var/lib/
chmod u=rwx,g=rx,o=rx ${PREFIX}/var/lib/

# file: var/lib/lrpkg/
chown root.root ${PREFIX}/var/lib/lrpkg/
chmod u=rwx,g=rx,o=rx ${PREFIX}/var/lib/lrpkg/

# file: var/lib/lrpkg/samba.version
chown root.root ${PREFIX}/var/lib/lrpkg/samba.version
chmod u=rw,g=r,o=r ${PREFIX}/var/lib/lrpkg/samba.version

# file: var/lib/lrpkg/samba.help
chown root.root ${PREFIX}/var/lib/lrpkg/samba.help
chmod u=rw,g=r,o=r ${PREFIX}/var/lib/lrpkg/samba.help

# file: var/lib/lrpkg/samba.list
chown root.root ${PREFIX}/var/lib/lrpkg/samba.list
chmod u=rw ${PREFIX}/var/lib/lrpkg/samba.list

# file: var/lib/lrpkg/samba.exclude.list
#chown root.root ${PREFIX}/var/lib/lrpkg/samba.exclude.list
#chmod u=rw ${PREFIX}/var/lib/lrpkg/samba.exclude.list

# file: var/lib/lrpkg/samba.conf
chown root.root ${PREFIX}/var/lib/lrpkg/samba.conf
chmod u=rw ${PREFIX}/var/lib/lrpkg/samba.conf
