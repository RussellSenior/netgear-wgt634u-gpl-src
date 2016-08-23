CC:=arm-elf-gcc

CFLAGS:=-g -Wall -Wcast-align -mapcs-32 -mcpu=arm9tdmi -fpic -fno-builtin -msingle-pic-base -msoft-float -Dlinux -D__linux__ -Dunix -I../../iptables/include -static -DEMBED -Wl,-elf2flt=\"-d\" -L../../iptables/libipq

nfqueue: nfqueue.o
	$(CC) $(CFLAGS) -o nfqueue nfqueue.o -lipq

nfqueue.o: ../nfqueue.c
	$(CC) $(CFLAGS) -o nfqueue.o -c ../nfqueue.c

clean:
	rm *.o

