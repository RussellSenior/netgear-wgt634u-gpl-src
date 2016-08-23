#! /bin/sh

## file to correct chown/chmod for package bridge
# generated 18.11.2002 02:44

# set PREFIX here
PREFIX=/home/changcs/usb-router-buildroot/build/packages/bridge

# use commandline to overwrite prefix

 if  [ "x$1" != "x" ] ; then 
PREFIX=/home/changcs/usb-router-buildroot/build/packages/bridge
fi


# just to be sure 
if [ "x$PREFIX" = "x" ] ; then
echo no prexix set, this is DANGEROUS, exiting
 exit 1
fi
exit 0
# file: etc/
chown root.root ${PREFIX}/etc/
chmod u=rwx,g=rx,o=rx ${PREFIX}/etc/

# file: etc/network/
chown root.root ${PREFIX}/etc/network/
chmod u=rwx,g=rx,o=rx ${PREFIX}/etc/network/

# file: etc/network/if-post-down.d/
chown root.root ${PREFIX}/etc/network/if-post-down.d/
chmod u=rwx,g=rx,o=rx ${PREFIX}/etc/network/if-post-down.d/

# file: etc/init.d
chown root.root ${PREFIX}/etc/init.d
chmod u=rwx,g=rx,o=rx ${PREFIX}/etc/init.d

# file: etc/init.d/ez-ipupd
chown root.root ${PREFIX}/etc/init.d/ez-ipupd
chmod u=rwx,g=rx,o=rx ${PREFIX}/etc/init.d/ez-ipupd

# file: etc/ez-ipupd.conf
chown root.root ${PREFIX}/etc/ez-ipupd.conf
chmod u=rw,g=r,o=r ${PREFIX}/etc/ez-ipupd.conf

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

# file: var/lib/lrpkg/bridge.version
chown root.root ${PREFIX}/var/lib/lrpkg/bridge.version
chmod u=rw,g=r,o=r ${PREFIX}/var/lib/lrpkg/bridge.version

# file: var/lib/lrpkg/bridge.help
chown root.root ${PREFIX}/var/lib/lrpkg/bridge.help
chmod u=rw,g=r,o=r ${PREFIX}/var/lib/lrpkg/bridge.help

# file: var/lib/lrpkg/bridge.list
chown root.root ${PREFIX}/var/lib/lrpkg/bridge.list
chmod u=rw ${PREFIX}/var/lib/lrpkg/bridge.list

# file: var/lib/lrpkg/bridge.conf
chown root.root ${PREFIX}/var/lib/lrpkg/bridge.conf
chmod u=rw ${PREFIX}/var/lib/lrpkg/bridge.conf
