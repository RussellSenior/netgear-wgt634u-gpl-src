BUILD_DIR:=${shell pwd}
LINUX_DIR:=$(BUILD_DIR)/linux-2.4.x
STAGING_DIR:=$(BUILD_DIR)/staging_dir
UCLIBC_DIR=$(BUILD_DIR)/uClibc
BUSYBOX_DIR:=$(BUILD_DIR)/busybox-0.60.5
ARCH:=mipsel
TARGET_CROSS:=$(STAGING_DIR)/bin/$(ARCH)-uclibc-
TARGET_DIR:=$(BUILD_DIR)
STRIP:=$(TARGET_CROSS)strip --remove-section=.comment --remove-section=.note
TINYLOGIN_DIR:=$(BUILD_DIR)/tinylogin-1.2
TINYLOGIN_CFLAGS:="-I $(TINYLOGIN_DIR)"
TARGET_CC1:=$(TARGET_CROSS)gcc
DEBIANUTILS_DIR:=$(BUILD_DIR)/debianutils-2.6.1
SYSVINIT_DIR:=$(BUILD_DIR)/sysvinit-2.84.orig
TARGET_CC:=$(STAGING_DIR)/usr/bin/gcc
IFUPDOWN_DIR:=$(BUILD_DIR)/ifupdown-0.6.4
NET-TOOLS_DIR:=$(BUILD_DIR)/net-tools-1.60
SYSKLOGD_DIR:=$(BUILD_DIR)/sysklogd-1.4.1
IPROUTE_DIR:=$(BUILD_DIR)/iproute2
SED_DIR:=$(BUILD_DIR)/sed-2.05
MTD_DIR:=$(BUILD_DIR)/mtd
CHECK_DIR:=$(BUILD_DIR)/check-4.2
SNARF_DIR:=$(BUILD_DIR)/snarf
PROCMAIL_DIR:=$(BUILD_DIR)/procmail-3.22
DHCPCD_DIR:=$(BUILD_DIR)/dhcpcd
IPTABLES_DIR:=$(BUILD_DIR)/iptables-1.2.7a
MAWK_DIR:=$(BUILD_DIR)/mawk-1.3.3
PPTP_DIR:=$(BUILD_DIR)/pptp
NFQUEUE_DIR:=$(BUILD_DIR)/nfqueue
EZIPUPD_DIR:=$(BUILD_DIR)/ez-ipupdate-3.0.11b8
ZEBRA_DIR:=$(BUILD_DIR)/quagga-0.96.4
BOA_DIR:=$(BUILD_DIR)/boa
BRIDGE_DIR:=$(BUILD_DIR)/bridge-utils-0.9.6.orig
SAMBA_DIR:=$(BUILD_DIR)/samba-1.9.16
DNSMASQ_DIR:=$(BUILD_DIR)/dnsmasq-1.18.orig
SMTPCLIENT_DIR:=$(BUILD_DIR)/smtpclient-1.0.0
PROFTPD_DIR:=$(BUILD_DIR)/proftpd-1.2.5
RP-PPPOE_DIR:=$(BUILD_DIR)/rp-pppoe-3.3
CPP_STAGING_DIR=/opt/toolchains/uclibc-crosstools-1.0.0
E2FSPROGS_DIR=$(BUILD_DIR)/e2fsprogs-1.27
UPNPSDK_DIR=$(BUILD_DIR)/upnpsdk
LINUXIGD_DIR=$(BUILD_DIR)/linuxigd
BPALOGIN_DIR:=$(BUILD_DIR)/bpalogin-2.0.2
WIRELESS_DIR:=$(BUILD_DIR)/wireless_tools.25

default: all

all: linux-dep staging busybox tinylogin debianutils sysvinit ifupdown net-tools \
     sysklogd iproute sed mtd-utils check snarf-utils procmail dhcpcd-utils \
     iptables mawk pptp-utils nfqueue-utils ez-ipupdate quagga boa-utils \
     bridge samba \
     dnsmasq smtpclient proftpd rp-pppoe e2fsprogs libupnpsdk \
     linuxigd-utils bpalogin wireless-utils linux-kernel

linux-dep:
	$(MAKE) -C $(LINUX_DIR) oldconfig include/linux/version.h
	make SRCBASE=$(BUILD_DIR)/broadcom-src -C $(LINUX_DIR) dep

staging:
	perl -i -p -e 's,^KERNEL_SOURCE=.*,KERNEL_SOURCE=\"$(LINUX_DIR)\",g' \
		$(UCLIBC_DIR)/.config
	perl -i -p -e 's,^DEVEL_PREFIX=.*,DEVEL_PREFIX=\"$(STAGING_DIR)\",g' \
		$(UCLIBC_DIR)/.config
	perl -i -p -e 's,^SYSTEM_DEVEL_PREFIX=.*,SYSTEM_DEVEL_PREFIX=\"$(STAGING_DIR)\",g' \
		$(UCLIBC_DIR)/.config
	perl -i -p -e 's,^DEVEL_TOOL_PREFIX=.*,DEVEL_TOOL_PREFIX=\"$(STAGING_DIR)/usr\",g' \
		$(UCLIBC_DIR)/.config
	$(MAKE) -C $(UCLIBC_DIR) oldconfig
	$(MAKE) -C $(UCLIBC_DIR)
	$(MAKE) -C $(UCLIBC_DIR) install
	perl -i -p -e 's,^.*asm/system\.h.*$$,,g' $(STAGING_DIR)/include/linux/spinlock.h

busybox:
	$(MAKE) CROSS="$(TARGET_CROSS)" PREFIX="$(TARGET_DIR)" -C $(BUSYBOX_DIR)

tinylogin:
	$(MAKE) LDSHARED="$(TARGET_CROSS)gcc --shared" CFLAGS_EXTRA=$(TINYLOGIN_CFLAGS) CC=$(TARGET_CC1) AR=$(TARGET_CROSS)ar STRIPTOOL=$(TARGET_CROSS)strip -C $(TINYLOGIN_DIR) all;

debianutils:
	$(MAKE) LDSHARED="$(TARGET_CROSS)gcc --shared" CFLAGS= CC=$(TARGET_CC1) -C $(DEBIANUTILS_DIR) all ;

sysvinit:
	$(MAKE) CC=$(TARGET_CC) -C $(SYSVINIT_DIR)/src all ;
	(cd $(SYSVINIT_DIR)/contrib ; $(TARGET_CC) -o start-stop-daemon start-stop-daemon.c; $(TARGET_CROSS)strip start-stop-daemon) 

ifupdown:
	$(MAKE) CC=$(TARGET_CC) -C $(IFUPDOWN_DIR) executables ;

net-tools:
	$(MAKE) CC=$(TARGET_CC) -C $(NET-TOOLS_DIR) arp ;
	$(MAKE) CC=$(TARGET_CC) -C $(NET-TOOLS_DIR) ifconfig route netstat mii-tool ;

sysklogd:
	$(MAKE) CC=$(TARGET_CC) -C $(SYSKLOGD_DIR) all;

iproute:
	$(MAKE) PATH=$(STAGING_DIR)/usr/bin:$(PATH) CC=gcc -C $(IPROUTE_DIR) LIBC_INCLUDE=$(STAGING_DIR)/include KERNEL_INCLUDE=$(LINUX_DIR)/include SUBDIRS="lib ip" all ;

sed:
	$(MAKE) PATH=$(STAGING_DIR)/usr/bin:$(PATH) CC=gcc -C $(SED_DIR) ;
	$(STRIP) $(SED_DIR)/sed

mtd-utils:
	$(MAKE) PATH=$(STAGING_DIR)/usr/bin:$(PATH) -C $(MTD_DIR)/util all;

check:
	(cd $(CHECK_DIR); $(TARGET_CC) -O3 -Wall -o check check.c)

snarf-utils:
	(export CC=gcc; $(MAKE) PATH=$(STAGING_DIR)/usr/bin:$(PATH) -C $(SNARF_DIR))

procmail:
	$(MAKE) PATH=$(STAGING_DIR)/usr/bin:$(PATH) LDFLAGS="-s" -C $(PROCMAIL_DIR)/src ../new/lockfile

dhcpcd-utils:
	$(MAKE) PATH=$(STAGING_DIR)/usr/bin:$(PATH) CC=gcc -C $(DHCPCD_DIR)

iptables:
	$(MAKE) PATH=$(STAGING_DIR)/usr/bin:$(PATH) BINDIR=/sbin DO_IPV6= LIBDIR=/lib MANDIR=/usr/share/man INCDIR=$(STAGING_DIR)/include KERNEL_DIR=$(LINUX_DIR) CC=gcc NO_SHARED_LIBS=1 -C $(IPTABLES_DIR) all

mawk:
	rm -f $(MAWK_DIR)/rexp/.done
	$(MAKE) PATH=$(STAGING_DIR)/usr/bin:$(PATH) CC=gcc CFLAGS="-g -Wall -O2" -C $(MAWK_DIR) mawk

pptp-utils:
	(export CC=gcc; $(MAKE) PATH=$(STAGING_DIR)/usr/bin:$(PATH) -C $(PPTP_DIR))

nfqueue-utils:
	(export CC=gcc; $(MAKE) PATH=$(STAGING_DIR)/usr/bin:$(PATH) CFLAGS="-g -O0 -static -I$(IPTABLES_DIR)/include -L$(IPTABLES_DIR)/libipq" -C $(NFQUEUE_DIR)/build)

ez-ipupdate:
	$(MAKE) PATH=$(STAGING_DIR)/usr/bin:$(PATH) CC=gcc -C $(EZIPUPD_DIR)

quagga:
	$(MAKE) PATH=$(STAGING_DIR)/usr/bin:$(PATH) CC=gcc -C $(ZEBRA_DIR)

boa-utils:
	(export CC=gcc; $(MAKE) PATH=$(STAGING_DIR)/usr/bin:$(PATH) -C $(BOA_DIR)/src)
	(export CC=gcc; $(MAKE) PATH=$(STAGING_DIR)/usr/bin:$(PATH) -C $(BOA_DIR)/proccgi proccgi)

bridge:
	(export CC=gcc; $(MAKE) PATH=$(STAGING_DIR)/usr/bin:$(PATH) -C $(BRIDGE_DIR))

samba:
	(export CC=gcc; $(MAKE) PATH=$(STAGING_DIR)/usr/bin:$(PATH) -C $(SAMBA_DIR)/source)

dnsmasq:
	$(MAKE) PATH=$(STAGING_DIR)/usr/bin:$(PATH) -C $(DNSMASQ_DIR)

smtpclient:
	(export CC=gcc; $(MAKE) PATH=$(STAGING_DIR)/usr/bin:$(PATH) -C $(SMTPCLIENT_DIR))

proftpd:
	(export CC=gcc; $(MAKE) PATH=$(STAGING_DIR)/usr/bin:$(PATH) -C $(PROFTPD_DIR))

rp-pppoe:
	(export CC=gcc; $(MAKE) PATH=$(STAGING_DIR)/usr/bin:$(PATH) -C $(RP-PPPOE_DIR)/src)

e2fsprogs:
	$(MAKE) CFLAGS= -C $(E2FSPROGS_DIR)/util subst
	-$(MAKE) CC=gcc PATH=$(CPP_STAGING_DIR)/mipsel-linux/bin:$(PATH) -C $(E2FSPROGS_DIR) libs

libupnpsdk:
	rm -f $(UPNPSDK_DIR)/upnp
	CC=gcc GCC=gcc PATH=$(CPP_STAGING_DIR)/mipsel-linux/bin:$(PATH) CFLAGS="" $(MAKE) WEB=1 UUID_INC=$(E2FSPROGS_DIR)/lib -C $(UPNPSDK_DIR)

linuxigd-utils:
	(cd $(UPNPSDK_DIR); ln -sf inc upnp)
	PATH=$(CPP_STAGING_DIR)/mipsel-linux/bin:$(PATH) $(MAKE) CFLAGS="" LIBS="-static -L$(UPNPSDK_DIR)/bin -lupnp -L$(E2FSPROGS_DIR)/lib -luuid -lpthread" INCLUDES=-I$(UPNPSDK_DIR)  -C $(LINUXIGD_DIR)

bpalogin:
	(export CC=gcc; $(MAKE) PATH=$(STAGING_DIR)/usr/bin:$(PATH) -C $(BPALOGIN_DIR))

wireless-utils:
	$(MAKE) PATH=$(STAGING_DIR)/usr/bin:$(PATH) -C $(WIRELESS_DIR)

linux-kernel:
	$(MAKE) SRCBASE=$(BUILD_DIR)/broadcom-src -C $(LINUX_DIR) zImage
