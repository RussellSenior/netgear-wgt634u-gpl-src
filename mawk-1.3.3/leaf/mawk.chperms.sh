#! /bin/sh

## file to correct chown/chmod for package mawk
# generated 18.11.2002 02:44

# set PREFIX here
PREFIX=/home/changcs/usb-router-buildroot/build/packages/mawk

# use commandline to overwrite prefix

 if  [ "x$1" != "x" ] ; then 
PREFIX=/home/changcs/usb-router-buildroot/build/packages/mawk
fi


# just to be sure 
if [ "x$PREFIX" = "x" ] ; then
echo no prexix set, this is DANGEROUS, exiting
 exit 1
fi

# file: usr/bin/
chown root.root ${PREFIX}/usr/bin/
chmod u=rwx,g=rx,o=rx ${PREFIX}/usr/bin/

# file: usr/bin/mawk
chown root.root ${PREFIX}/usr/bin/mawk
chmod u=rwx,g=rx,o=rx ${PREFIX}/usr/bin/mawk

# file: usr/bin/awk
chown root.root ${PREFIX}/usr/bin/awk
chmod u=rwx,g=rx,o=rx ${PREFIX}/usr/bin/awk

# file: var/
chown root.root ${PREFIX}/var/
chmod u=rwx,g=rx,o=rx ${PREFIX}/var/

# file: var/lib/
chown root.root ${PREFIX}/var/lib/
chmod u=rwx,g=rx,o=rx ${PREFIX}/var/lib/

# file: var/lib/lrpkg/
chown root.root ${PREFIX}/var/lib/lrpkg/
chmod u=rwx,g=rx,o=rx ${PREFIX}/var/lib/lrpkg/

# file: var/lib/lrpkg/mawk.version
chown root.root ${PREFIX}/var/lib/lrpkg/mawk.version
chmod u=rw,g=r,o=r ${PREFIX}/var/lib/lrpkg/mawk.version

# file: var/lib/lrpkg/mawk.list
chown root.root ${PREFIX}/var/lib/lrpkg/mawk.list
chmod u=rw ${PREFIX}/var/lib/lrpkg/mawk.list
