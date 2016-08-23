CC:=gcc

#CFLAGS_INC:=-I../../iptables/include
#CFLAGS_LIB:=-L../../iptables/libipq
CFLAGS:=-g -O0 -static $(CFLAGS_INC) $(CFLAGS_LIB)

nfqueue: nfqueue.o
	$(CC) $(CFLAGS) -o nfqueue nfqueue.o -lipq

nfqueue.o: ../nfqueue.c
	$(CC) $(CFLAGS) -o nfqueue.o -c ../nfqueue.c

clean:
	rm *.o

