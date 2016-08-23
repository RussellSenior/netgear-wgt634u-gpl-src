#! /bin/sh

## file to correct chown/chmod for package nfqueue
# generated 3.5.2004 10:00

# set PREFIX here
PREFIX=/home/changcs/usb-router-buildroot/build/packages/nfqueue

# use commandline to overwrite prefix

if  [ "x$1" != "x" ] ; then 
PREFIX=/home/changcs/usb-router-buildroot/build/packages/nfqueue
fi


# just to be sure 
if [ "x$PREFIX" = "x" ] ; then
echo no prexix set, this is DANGEROUS, exiting
 exit 1
fi

# file: etc/
chown root.root ${PREFIX}/etc/
chmod u=rwx,g=rx,o=rx ${PREFIX}/etc/

# file: etc/init.d/
chown root.root ${PREFIX}/etc/init.d/
chmod u=rwx,g=rx,o=rx ${PREFIX}/etc/init.d/

# file: etc/init.d/nfqueue
chown root.root ${PREFIX}/etc/init.d/nfqueue
chmod u=rwx,g=rx,o=rx ${PREFIX}/etc/init.d/nfqueue

# file: etc/shorewall/
chown root.root ${PREFIX}/etc/shorewall/
chmod u=rwx,g=rx,o=rx ${PREFIX}/etc/shorewall/

# file: etc/shorewall/start
chown root.root ${PREFIX}/etc/shorewall/start
chmod u=rw,g=r,o=r ${PREFIX}/etc/shorewall/start

# file: etc/nfqueue/
chown root.root ${PREFIX}/etc/nfqueue/
chmod u=rwx,g=rx,o=rx ${PREFIX}/etc/nfqueue/

# file: etc/nfqueue/nfqueue.conf
chown root.root ${PREFIX}/etc/nfqueue/nfqueue.conf
chmod u=rw,g=r,o=r ${PREFIX}/etc/nfqueue/nfqueue.conf

# file: etc/nfqueue/http_response_page
chown root.root ${PREFIX}/etc/nfqueue/http_response_page
chmod u=rw,g=r,o=r ${PREFIX}/etc/nfqueue/http_response_page

# file: etc/nfqueue/blocking_schedule_on
chown root.root ${PREFIX}/etc/nfqueue/blocking_schedule_on
chmod u=rwx,g=rx,o=rx ${PREFIX}/etc/nfqueue/blocking_schedule_on

# file: etc/nfqueue/blocking_schedule_off
chown root.root ${PREFIX}/etc/nfqueue/blocking_schedule_off
chmod u=rwx,g=rx,o=rx ${PREFIX}/etc/nfqueue/blocking_schedule_off

# file: etc/cron.d/
chown root.root ${PREFIX}/etc/cron.d/
chmod u=rwx,g=rx,o=rx ${PREFIX}/etc/cron.d/

# file: etc/cron.d/nfqueue
chown root.root ${PREFIX}/etc/cron.d/nfqueue
chmod u=rw,g=r,o=r ${PREFIX}/etc/cron.d/nfqueue

# file: sbin/
chown root.root ${PREFIX}/sbin/
chmod u=rwx,g=rx,o=rx ${PREFIX}/sbin/

# file: sbin/nfqueue
chown root.root ${PREFIX}/sbin/nfqueue
chmod u=rwx,g=rx,o=rx ${PREFIX}/sbin/nfqueue

# file: var/
chown root.root ${PREFIX}/var/
chmod u=rwx,g=rx,o=rx ${PREFIX}/var/

# file: var/lib/
chown root.root ${PREFIX}/var/lib/
chmod u=rwx,g=rx,o=rx ${PREFIX}/var/lib/

# file: var/lib/lrpkg/
chown root.root ${PREFIX}/var/lib/lrpkg/
chmod u=rwx,g=rx,o=rx ${PREFIX}/var/lib/lrpkg/

# file: var/lib/lrpkg/nfqueue.version
chown root.root ${PREFIX}/var/lib/lrpkg/nfqueue.version
chmod u=rw,g=r,o=r ${PREFIX}/var/lib/lrpkg/nfqueue.version

# file: var/lib/lrpkg/nfqueue.help
chown root.root ${PREFIX}/var/lib/lrpkg/nfqueue.help
chmod u=rw,g=r,o=r ${PREFIX}/var/lib/lrpkg/nfqueue.help

# file: var/lib/lrpkg/nfqueue.list
chown root.root ${PREFIX}/var/lib/lrpkg/nfqueue.list
chmod u=rw ${PREFIX}/var/lib/lrpkg/nfqueue.list

# file: var/lib/lrpkg/nfqueue.conf
chown root.root ${PREFIX}/var/lib/lrpkg/nfqueue.conf
chmod u=rw ${PREFIX}/var/lib/lrpkg/nfqueue.conf
