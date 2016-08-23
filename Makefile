BUILD_DIR:=${shell pwd}
STAGING_DIR:=$(BUILD_DIR)/staging_dir
ARCH:=mipsel
TARGET_CC:=$(STAGING_DIR)/usr/bin/gcc
TARGET_CROSS:=$(STAGING_DIR)/bin/$(ARCH)-uclibc-
STRIP:=$(TARGET_CROSS)strip --remove-section=.comment --remove-section=.note
TARGET_CC1:=$(TARGET_CROSS)gcc
CPP_STAGING_DIR=/opt/toolchains/uclibc-crosstools-1.0.0

LINUX_DIR:=$(BUILD_DIR)/linux-2.4.x
BROADCOM_SRC_DIR:=$(BUILD_DIR)/broadcom-src
UCLIBC_DIR=$(BUILD_DIR)/uClibc
BUSYBOX_DIR:=$(BUILD_DIR)/busybox-0.60.5
TARGET_DIR:=$(BUILD_DIR)
TINYLOGIN_DIR:=$(BUILD_DIR)/tinylogin-1.2
TINYLOGIN_CFLAGS:="-I $(TINYLOGIN_DIR)"
DEBIANUTILS_DIR:=$(BUILD_DIR)/debianutils-2.6.1
SYSVINIT_DIR:=$(BUILD_DIR)/sysvinit-2.84.orig
IFUPDOWN_DIR:=$(BUILD_DIR)/ifupdown-0.6.4
NET_TOOLS_DIR:=$(BUILD_DIR)/net-tools-1.60
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
ZEBRA_DIR:=$(BUILD_DIR)/quagga-0.96.5
BOA_DIR:=$(BUILD_DIR)/boa
BRIDGE_DIR:=$(BUILD_DIR)/bridge-utils-0.9.6.orig
SAMBA_DIR:=$(BUILD_DIR)/samba-1.9.16
DNSMASQ_DIR:=$(BUILD_DIR)/dnsmasq-1.18.orig
SMTPCLIENT_DIR:=$(BUILD_DIR)/smtpclient-1.0.0
PROFTPD_DIR:=$(BUILD_DIR)/proftpd
RP_PPPOE_DIR:=$(BUILD_DIR)/rp-pppoe-3.3
E2FSPROGS_DIR=$(BUILD_DIR)/e2fsprogs-1.27
UPNPSDK_DIR=$(BUILD_DIR)/upnpsdk
LINUXIGD_DIR=$(BUILD_DIR)/linuxigd
BPALOGIN_DIR:=$(BUILD_DIR)/bpalogin-2.0.2
WIRELESS_DIR:=$(BUILD_DIR)/wireless_tools.25
FILE_DIR:=$(BUILD_DIR)/file-3.37
AUTOFS_DIR:=$(BUILD_DIR)/autofs-4.1.3

default: all

all: linux-dep staging busybox tinylogin \
     debianutils sysvinit ifupdown net-tools \
     sysklogd iproute sed mtd-utils check \
     snarf-utils procmail dhcpcd-utils \
     iptables mawk pptp-utils nfqueue-utils \
     ez-ipupdate quagga boa-utils \
     bridge samba \
     dnsmasq smtpclient proftpd rp-pppoe \
     e2fsprogs libupnpsdk \
     linuxigd-utils bpalogin wireless-utils \
     file autofs linux-kernel

clean: busybox-clean tinylogin-clean \
     debianutils-clean sysvinit-clean ifupdown-clean net-tools-clean \
     sysklogd-clean iproute-clean sed-clean mtd-utils-clean \
     check-clean snarf-utils-clean procmail-clean dhcpcd-utils-clean \
     iptables-clean mawk-clean pptp-utils-clean nfqueue-utils-clean \
     ez-ipupdate-clean quagga-clean boa-utils-clean \
     bridge-clean samba-clean \
     dnsmasq-clean smtpclient-clean proftpd-clean rp-pppoe-clean \
     e2fsprogs-clean libupnpsdk-clean \
     linuxigd-utils-clean bpalogin-clean wireless-utils-clean \
     file-clean autofs-clean linux-kernel-clean staging-clean

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

staging-clean:
	rm -rf $(STAGING_DIR)
	$(MAKE) -C $(UCLIBC_DIR) clean

busybox:
	$(MAKE) CROSS="$(TARGET_CROSS)" PREFIX="$(TARGET_DIR)" -C $(BUSYBOX_DIR)

busybox-clean:
	$(MAKE) -C $(BUSYBOX_DIR) clean

tinylogin:
	$(MAKE) LDSHARED="$(TARGET_CROSS)gcc --shared" CFLAGS_EXTRA=$(TINYLOGIN_CFLAGS) CC=$(TARGET_CC1) AR=$(TARGET_CROSS)ar STRIPTOOL=$(TARGET_CROSS)strip -C $(TINYLOGIN_DIR) all;

tinylogin-clean:
	$(MAKE) LDSHARED="$(TARGET_CROSS)gcc --shared" CFLAGS_EXTRA=$(TINYLOGIN_CFLAGS) CC=$(TARGET_CC1) AR=$(TARGET_CROSS)ar STRIPTOOL=$(TARGET_CROSS)strip -C $(TINYLOGIN_DIR) clean;

debianutils:
	$(MAKE) LDSHARED="$(TARGET_CROSS)gcc --shared" CFLAGS= CC=$(TARGET_CC1) -C $(DEBIANUTILS_DIR) all ;

debianutils-clean:
	$(MAKE) LDSHARED="$(TARGET_CROSS)gcc --shared" CFLAGS= CC=$(TARGET_CC1) -C $(DEBIANUTILS_DIR) clean ;

sysvinit:
	$(MAKE) CC=$(TARGET_CC) -C $(SYSVINIT_DIR)/src all ;
	(cd $(SYSVINIT_DIR)/contrib ; $(TARGET_CC) -o start-stop-daemon start-stop-daemon.c; $(TARGET_CROSS)strip start-stop-daemon) 

sysvinit-clean:
	$(MAKE) CC=$(TARGET_CC) -C $(SYSVINIT_DIR)/src clean ;

ifupdown:
	$(MAKE) CC=$(TARGET_CC) -C $(IFUPDOWN_DIR) executables ;

ifupdown-clean:
	$(MAKE) CC=$(TARGET_CC) -C $(IFUPDOWN_DIR) clean ;

net-tools:
	$(MAKE) CC=$(TARGET_CC) -C $(NET_TOOLS_DIR) arp ;
	$(MAKE) CC=$(TARGET_CC) -C $(NET_TOOLS_DIR) ifconfig route netstat mii-tool ;

net-tools-clean:
	$(MAKE) CC=$(TARGET_CC) -C $(NET_TOOLS_DIR) clean ;

sysklogd:
	$(MAKE) CC=$(TARGET_CC) -C $(SYSKLOGD_DIR) all;

sysklogd-clean:
	$(MAKE) CC=$(TARGET_CC) -C $(SYSKLOGD_DIR) clean;

iproute:
	$(MAKE) PATH=$(STAGING_DIR)/usr/bin:$(PATH) CC=gcc -C $(IPROUTE_DIR) LIBC_INCLUDE=$(STAGING_DIR)/include KERNEL_INCLUDE=$(LINUX_DIR)/include SUBDIRS="lib ip" all ;

iproute-clean:
	$(MAKE) PATH=$(STAGING_DIR)/usr/bin:$(PATH) CC=gcc -C $(IPROUTE_DIR) LIBC_INCLUDE=$(STAGING_DIR)/include KERNEL_INCLUDE=$(LINUX_DIR)/include SUBDIRS="lib ip" clean ;

sed:
	$(MAKE) PATH=$(STAGING_DIR)/usr/bin:$(PATH) CC=gcc -C $(SED_DIR) ;
	$(STRIP) $(SED_DIR)/sed

sed-clean:
	$(MAKE) PATH=$(STAGING_DIR)/usr/bin:$(PATH) CC=gcc -C $(SED_DIR) clean ;

mtd-utils:
	$(MAKE) PATH=$(STAGING_DIR)/usr/bin:$(PATH) -C $(MTD_DIR)/util all;

mtd-utils-clean:
	$(MAKE) PATH=$(STAGING_DIR)/usr/bin:$(PATH) -C $(MTD_DIR)/util clean;

check:
	(cd $(CHECK_DIR); $(TARGET_CC) -O3 -Wall -o check check.c)

check-clean:
	(cd $(CHECK_DIR); rm -f check)

snarf-utils:
	(export CC=gcc; $(MAKE) PATH=$(STAGING_DIR)/usr/bin:$(PATH) -C $(SNARF_DIR))

snarf-utils-clean:
	(export CC=gcc; $(MAKE) PATH=$(STAGING_DIR)/usr/bin:$(PATH) -C $(SNARF_DIR) clean)

procmail:
	$(MAKE) PATH=$(STAGING_DIR)/usr/bin:$(PATH) LDFLAGS="-s" -C $(PROCMAIL_DIR)/src ../new/lockfile

procmail-clean:
	find $(PROCMAIL_DIR) -name "*.o" | xargs rm -f

dhcpcd-utils:
	$(MAKE) PATH=$(STAGING_DIR)/usr/bin:$(PATH) CC=gcc -C $(DHCPCD_DIR)

dhcpcd-utils-clean:
	$(MAKE) PATH=$(STAGING_DIR)/usr/bin:$(PATH) CC=gcc -C $(DHCPCD_DIR) clean

iptables:
	$(MAKE) PATH=$(STAGING_DIR)/usr/bin:$(PATH) BINDIR=/sbin DO_IPV6= LIBDIR=/lib MANDIR=/usr/share/man INCDIR=$(STAGING_DIR)/include KERNEL_DIR=$(LINUX_DIR) CC=gcc NO_SHARED_LIBS=1 -C $(IPTABLES_DIR) all

iptables-clean:
	$(MAKE) PATH=$(STAGING_DIR)/usr/bin:$(PATH) BINDIR=/sbin DO_IPV6= LIBDIR=/lib MANDIR=/usr/share/man INCDIR=$(STAGING_DIR)/include KERNEL_DIR=$(LINUX_DIR) CC=gcc NO_SHARED_LIBS=1 -C $(IPTABLES_DIR) clean
	-find $(IPTABLES_DIR) -iname "*.d" | xargs rm

mawk:
	rm -f $(MAWK_DIR)/rexp/.done
	$(MAKE) PATH=$(STAGING_DIR)/usr/bin:$(PATH) CC=gcc CFLAGS="-g -Wall -O2" -C $(MAWK_DIR) mawk

mawk-clean:
	$(MAKE) PATH=$(STAGING_DIR)/usr/bin:$(PATH) CC=gcc CFLAGS="-g -Wall -O2" -C $(MAWK_DIR) clean

pptp-utils:
	(export CC=gcc; $(MAKE) PATH=$(STAGING_DIR)/usr/bin:$(PATH) -C $(PPTP_DIR))

pptp-utils-clean:
	(export CC=gcc; $(MAKE) PATH=$(STAGING_DIR)/usr/bin:$(PATH) -C $(PPTP_DIR) clean)

nfqueue-utils:
	(export CC=gcc; $(MAKE) PATH=$(STAGING_DIR)/usr/bin:$(PATH) CFLAGS="-g -O0 -static -I$(IPTABLES_DIR)/include -L$(IPTABLES_DIR)/libipq" -C $(NFQUEUE_DIR)/build)

nfqueue-utils-clean:
	-(export CC=gcc; $(MAKE) PATH=$(STAGING_DIR)/usr/bin:$(PATH) CFLAGS="-g -O0 -static -I$(IPTABLES_DIR)/include -L$(IPTABLES_DIR)/libipq" -C $(NFQUEUE_DIR)/build clean)

ez-ipupdate:
	$(MAKE) PATH=$(STAGING_DIR)/usr/bin:$(PATH) CC=gcc -C $(EZIPUPD_DIR)

ez-ipupdate-clean:
	$(MAKE) PATH=$(STAGING_DIR)/usr/bin:$(PATH) CC=gcc -C $(EZIPUPD_DIR) clean

quagga:
	$(MAKE) PATH=$(STAGING_DIR)/usr/bin:$(PATH) CC=gcc -C $(ZEBRA_DIR)

quagga-clean:
	$(MAKE) PATH=$(STAGING_DIR)/usr/bin:$(PATH) CC=gcc -C $(ZEBRA_DIR) clean

boa-utils:
	(export CC=gcc; $(MAKE) PATH=$(STAGING_DIR)/usr/bin:$(PATH) -C $(BOA_DIR)/src)
	(export CC=gcc; $(MAKE) PATH=$(STAGING_DIR)/usr/bin:$(PATH) -C $(BOA_DIR)/proccgi proccgi)

boa-utils-clean:
	(export CC=gcc; $(MAKE) PATH=$(STAGING_DIR)/usr/bin:$(PATH) -C $(BOA_DIR)/src clean)
	(export CC=gcc; $(MAKE) PATH=$(STAGING_DIR)/usr/bin:$(PATH) -C $(BOA_DIR)/proccgi clean)

bridge:
	(export CC=gcc; $(MAKE) PATH=$(STAGING_DIR)/usr/bin:$(PATH) -C $(BRIDGE_DIR))

bridge-clean:
	(export CC=gcc; $(MAKE) PATH=$(STAGING_DIR)/usr/bin:$(PATH) -C $(BRIDGE_DIR) clean)

samba:
	(export CC=gcc; $(MAKE) PATH=$(STAGING_DIR)/usr/bin:$(PATH) -C $(SAMBA_DIR)/source)

samba-clean:
	(export CC=gcc; $(MAKE) PATH=$(STAGING_DIR)/usr/bin:$(PATH) -C $(SAMBA_DIR)/source clean)

dnsmasq:
	$(MAKE) PATH=$(STAGING_DIR)/usr/bin:$(PATH) -C $(DNSMASQ_DIR)

dnsmasq-clean:
	$(MAKE) PATH=$(STAGING_DIR)/usr/bin:$(PATH) -C $(DNSMASQ_DIR) clean

smtpclient:
	(export CC=gcc; $(MAKE) PATH=$(STAGING_DIR)/usr/bin:$(PATH) -C $(SMTPCLIENT_DIR))

smtpclient-clean:
	(export CC=gcc; $(MAKE) PATH=$(STAGING_DIR)/usr/bin:$(PATH) -C $(SMTPCLIENT_DIR) clean)

proftpd:
	(export CC=gcc; $(MAKE) PATH=$(STAGING_DIR)/usr/bin:$(PATH) -C $(PROFTPD_DIR))

proftpd-clean:
	(export CC=gcc; $(MAKE) PATH=$(STAGING_DIR)/usr/bin:$(PATH) -C $(PROFTPD_DIR) clean)

rp-pppoe:
	(export CC=gcc; $(MAKE) PATH=$(STAGING_DIR)/usr/bin:$(PATH) -C $(RP_PPPOE_DIR)/src)

rp-pppoe-clean:
	(export CC=gcc; $(MAKE) PATH=$(STAGING_DIR)/usr/bin:$(PATH) -C $(RP_PPPOE_DIR)/src clean)

e2fsprogs:
	$(MAKE) CFLAGS= -C $(E2FSPROGS_DIR)/util subst
	$(MAKE) CC=gcc PATH=$(CPP_STAGING_DIR)/mipsel-linux/bin:$(PATH) -C $(E2FSPROGS_DIR) libs

e2fsprogs-clean:
	-find $(E2FSPROGS_DIR) -iname "*.o" | xargs rm 

libupnpsdk:
	rm -f $(UPNPSDK_DIR)/upnp
	CC=gcc GCC=gcc PATH=$(CPP_STAGING_DIR)/mipsel-linux/bin:$(PATH) CFLAGS="" $(MAKE) WEB=1 UUID_INC=$(E2FSPROGS_DIR)/lib -C $(UPNPSDK_DIR)

libupnpsdk-clean:
	rm -f $(UPNPSDK_DIR)/upnp
	CC=gcc GCC=gcc PATH=$(CPP_STAGING_DIR)/mipsel-linux/bin:$(PATH) CFLAGS="" $(MAKE) WEB=1 UUID_INC=$(E2FSPROGS_DIR)/lib -C $(UPNPSDK_DIR) clean

linuxigd-utils:
	(cd $(UPNPSDK_DIR); ln -sf inc upnp)
	PATH=$(CPP_STAGING_DIR)/mipsel-linux/bin:$(PATH) $(MAKE) CFLAGS="" LIBS="-static -L$(UPNPSDK_DIR)/bin -lupnp -L$(E2FSPROGS_DIR)/lib -luuid -lpthread" INCLUDES=-I$(UPNPSDK_DIR)  -C $(LINUXIGD_DIR)

linuxigd-utils-clean:
	PATH=$(CPP_STAGING_DIR)/mipsel-linux/bin:$(PATH) $(MAKE) CFLAGS="" LIBS="-static -L$(UPNPSDK_DIR)/bin -lupnp -L$(E2FSPROGS_DIR)/lib -luuid -lpthread" INCLUDES=-I$(UPNPSDK_DIR)  -C $(LINUXIGD_DIR) clean

bpalogin:
	(export CC=gcc; $(MAKE) PATH=$(STAGING_DIR)/usr/bin:$(PATH) -C $(BPALOGIN_DIR))

bpalogin-clean:
	(export CC=gcc; $(MAKE) PATH=$(STAGING_DIR)/usr/bin:$(PATH) -C $(BPALOGIN_DIR) clean)

wireless-utils:
	$(MAKE) PATH=$(STAGING_DIR)/usr/bin:$(PATH) -C $(WIRELESS_DIR)

wireless-utils-clean:
	$(MAKE) PATH=$(STAGING_DIR)/usr/bin:$(PATH) -C $(WIRELESS_DIR) realclean

file:
	$(MAKE) PATH=$(STAGING_DIR)/usr/bin:$(PATH) -C $(FILE_DIR)

file-clean:
	$(MAKE) PATH=$(STAGING_DIR)/usr/bin:$(PATH) -C $(FILE_DIR) clean

autofs:
	$(MAKE) PATH=$(STAGING_DIR)/usr/bin:$(PATH) -C $(AUTOFS_DIR)

autofs-clean:
	$(MAKE) PATH=$(STAGING_DIR)/usr/bin:$(PATH) -C $(AUTOFS_DIR) clean

linux-kernel:
	$(MAKE) SRCBASE=$(BUILD_DIR)/broadcom-src -C $(LINUX_DIR) zImage

linux-kernel-clean:
	cp $(LINUX_DIR)/.config $(LINUX_DIR)/kernel_config_of_wgt634u
	$(MAKE) SRCBASE=$(BUILD_DIR)/$(BROADCOM_SRC_DIR) -C $(LINUX_DIR) distclean
	mv $(LINUX_DIR)/kernel_config_of_wgt634u $(LINUX_DIR)/.config

copy_gpl_source:
	( cp -rf $(subst $(BUILD_DIR),$(GPL_SOURCE_DIR),$(LINUX_DIR)) $(BUILD_DIR); \
	  perl -i -p -e "s/CONFIG_IPSEC=m/# CONFIG_IPSEC is not set/g;" $(LINUX_DIR)/.config; \
	  perl -i -p -e "s/ipsec//g;" $(LINUX_DIR)/net/Makefile; )
	( cp -rf $(subst $(BUILD_DIR),$(GPL_SOURCE_DIR),$(BROADCOM_SRC_DIR)) $(BUILD_DIR); \
	  rm -rf $(BROADCOM_SRC_DIR)/et $(BROADCOM_SRC_DIR)/switch $(BROADCOM_SRC_DIR)/il; \
	  mv $(BROADCOM_SRC_DIR)/et-binmod $(BROADCOM_SRC_DIR)/et; \
	  mv $(BROADCOM_SRC_DIR)/switch-binmod $(BROADCOM_SRC_DIR)/switch; )
	cp -a $(subst $(BUILD_DIR),$(GPL_SOURCE_DIR),$(UCLIBC_DIR)) $(BUILD_DIR)
	cp -a $(subst $(BUILD_DIR),$(GPL_SOURCE_DIR),$(BUSYBOX_DIR)) $(BUILD_DIR)
	cp -a $(subst $(BUILD_DIR),$(GPL_SOURCE_DIR),$(TINYLOGIN_DIR)) $(BUILD_DIR)
	cp -a $(subst $(BUILD_DIR),$(GPL_SOURCE_DIR),$(DEBIANUTILS_DIR)) $(BUILD_DIR)
	cp -a $(subst $(BUILD_DIR),$(GPL_SOURCE_DIR),$(SYSVINIT_DIR)) $(BUILD_DIR)
	cp -a $(subst $(BUILD_DIR),$(GPL_SOURCE_DIR),$(IFUPDOWN_DIR)) $(BUILD_DIR)
	cp -a $(subst $(BUILD_DIR),$(GPL_SOURCE_DIR),$(NET_TOOLS_DIR)) $(BUILD_DIR)
	cp -a $(subst $(BUILD_DIR),$(GPL_SOURCE_DIR),$(SYSKLOGD_DIR)) $(BUILD_DIR)
	cp -a $(subst $(BUILD_DIR),$(GPL_SOURCE_DIR),$(IPROUTE_DIR)) $(BUILD_DIR)
	cp -a $(subst $(BUILD_DIR),$(GPL_SOURCE_DIR),$(SED_DIR)) $(BUILD_DIR)
	cp -a $(subst $(BUILD_DIR),$(GPL_SOURCE_DIR),$(MTD_DIR)) $(BUILD_DIR)
	cp -a $(subst $(BUILD_DIR),$(GPL_SOURCE_DIR),$(CHECK_DIR)) $(BUILD_DIR)
	cp -a $(subst $(BUILD_DIR),$(GPL_SOURCE_DIR),$(SNARF_DIR)) $(BUILD_DIR)
	cp -a $(subst $(BUILD_DIR),$(GPL_SOURCE_DIR),$(PROCMAIL_DIR)) $(BUILD_DIR)
	cp -a $(subst $(BUILD_DIR),$(GPL_SOURCE_DIR),$(DHCPCD_DIR)) $(BUILD_DIR)
	cp -a $(subst $(BUILD_DIR),$(GPL_SOURCE_DIR),$(IPTABLES_DIR)) $(BUILD_DIR)
	cp -a $(subst $(BUILD_DIR),$(GPL_SOURCE_DIR),$(MAWK_DIR)) $(BUILD_DIR)
	cp -a $(subst $(BUILD_DIR),$(GPL_SOURCE_DIR),$(PPTP_DIR)) $(BUILD_DIR)
	cp -a $(subst $(BUILD_DIR),$(GPL_SOURCE_DIR),$(NFQUEUE_DIR)) $(BUILD_DIR)
	cp -a $(subst $(BUILD_DIR),$(GPL_SOURCE_DIR),$(EZIPUPD_DIR)) $(BUILD_DIR)
	cp -a $(subst $(BUILD_DIR),$(GPL_SOURCE_DIR),$(ZEBRA_DIR)) $(BUILD_DIR)
	cp -a $(subst $(BUILD_DIR),$(GPL_SOURCE_DIR),$(BOA_DIR)) $(BUILD_DIR)
	cp -a $(subst $(BUILD_DIR),$(GPL_SOURCE_DIR),$(BRIDGE_DIR)) $(BUILD_DIR)
	cp -a $(subst $(BUILD_DIR),$(GPL_SOURCE_DIR),$(SAMBA_DIR)) $(BUILD_DIR)
	cp -a $(subst $(BUILD_DIR),$(GPL_SOURCE_DIR),$(DNSMASQ_DIR)) $(BUILD_DIR)
	cp -a $(subst $(BUILD_DIR),$(GPL_SOURCE_DIR),$(SMTPCLIENT_DIR)) $(BUILD_DIR)
	cp -a $(subst $(BUILD_DIR),$(GPL_SOURCE_DIR),$(PROFTPD_DIR)) $(BUILD_DIR)
	cp -a $(subst $(BUILD_DIR),$(GPL_SOURCE_DIR),$(RP_PPPOE_DIR)) $(BUILD_DIR)
	cp -a $(subst $(BUILD_DIR),$(GPL_SOURCE_DIR),$(E2FSPROGS_DIR)) $(BUILD_DIR)
	cp -a $(subst $(BUILD_DIR),$(GPL_SOURCE_DIR),$(UPNPSDK_DIR)) $(BUILD_DIR)
	cp -a $(subst $(BUILD_DIR),$(GPL_SOURCE_DIR),$(LINUXIGD_DIR)) $(BUILD_DIR)
	cp -a $(subst $(BUILD_DIR),$(GPL_SOURCE_DIR),$(BPALOGIN_DIR)) $(BUILD_DIR)
	cp -a $(subst $(BUILD_DIR),$(GPL_SOURCE_DIR),$(WIRELESS_DIR)) $(BUILD_DIR)
	cp -a $(subst $(BUILD_DIR),$(GPL_SOURCE_DIR),$(FILE_DIR)) $(BUILD_DIR)
	cp -a $(subst $(BUILD_DIR),$(GPL_SOURCE_DIR),$(AUTOFS_DIR)) $(BUILD_DIR)

