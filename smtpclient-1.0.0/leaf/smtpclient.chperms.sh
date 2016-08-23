#! /bin/sh

## file to correct chown/chmod for package smtpclient
# generated 12.29.2003 15:44

# set PREFIX here
PREFIX=/home/changcs/usb-router-buildroot/build/packages/smtpclient

# use commandline to overwrite prefix

 if  [ "x$1" != "x" ] ; then 
PREFIX=/home/changcs/usb-router-buildroot/build/packages/smtpclient
fi


# just to be sure 
if [ "x$PREFIX" = "x" ] ; then
echo no prexix set, this is DANGEROUS, exiting
 exit 1
fi

# file: usr/
chown root.root ${PREFIX}/usr/
chmod u=rwx,g=rx,o=rx ${PREFIX}/usr/

# file: usr/bin/
chown root.root ${PREFIX}/usr/bin/
chmod u=rwx,g=rx,o=rx ${PREFIX}/usr/bin/

# file: usr/bin/smtpclient
chown root.root ${PREFIX}/usr/bin/smtpclient
chmod u=rwx,g=rx,o=rx ${PREFIX}/usr/bin/smtpclient

# file: var/
chown root.root ${PREFIX}/var/
chmod u=rwx,g=rx,o=rx ${PREFIX}/var/

# file: var/lib/
chown root.root ${PREFIX}/var/lib/
chmod u=rwx,g=rx,o=rx ${PREFIX}/var/lib/

# file: var/lib/lrpkg/
chown root.root ${PREFIX}/var/lib/lrpkg/
chmod u=rwx,g=rx,o=rx ${PREFIX}/var/lib/lrpkg/

# file: var/lib/lrpkg/smtpclient.version
chown root.root ${PREFIX}/var/lib/lrpkg/smtpclient.version
chmod u=rw,g=r,o=r ${PREFIX}/var/lib/lrpkg/smtpclient.version

# file: var/lib/lrpkg/smtpclient.help
chown root.root ${PREFIX}/var/lib/lrpkg/smtpclient.help
chmod u=rw,g=r,o=r ${PREFIX}/var/lib/lrpkg/smtpclient.help

# file: var/lib/lrpkg/smtpclient.list
chown root.root ${PREFIX}/var/lib/lrpkg/smtpclient.list
chmod u=rw ${PREFIX}/var/lib/lrpkg/smtpclient.list

