
# -*- sh -*-

# $Id: Makefile,v 1.34 2003/06/20 13:58:46 dwmw2 Exp $

#CROSS=arm-linux-
CC := $(CROSS)gcc
CFLAGS += -I../include -O2 -Wall -Wstrict-prototypes -Wshadow

TARGETS = ftl_format erase eraseall  nanddump doc_loadbios \
	mkfs.jffs ftl_check lock unlock \
	einfo mtd_debug fcp nandwrite jffs2dump \
	#jffs2reader nftldump nftl_format

SYMLINKS = crc32.h crc32.c compr_rtime.c

all: $(TARGETS)

clean:
	rm -f *.o $(TARGETS) $(SYMLINKS)

mkfs.jffs2.o eraseall.o crc32.o: crc32.h

$(SYMLINKS):
	ln -sf ../fs/jffs2/$@ $@

mkfs.jffs2: crc32.o compr_rtime.o mkfs.jffs2.o compr_zlib.o
	$(CC) $(LDFLAGS) -o $@ $^ -lz

eraseall: crc32.o eraseall.o
	$(CC) $(LDFLAGS) -o $@ $^

jffs2reader: jffs2reader.o
	$(CC) $(LDFLAGS) -o $@ $^ -lz 

jffs2dump: jffs2dump.o crc32.o
	$(CC) $(LDFLAGS) -o $@ $^

