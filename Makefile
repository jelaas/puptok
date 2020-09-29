CC=musl-gcc-x86_32
CFLAGS=-Wall
all:	pup
pup:	pup.o tok.o
rpm:	pup
	bar -c --license=GPLv2+ --arch=noarch --name pup pup-1.4-1.rpm --prefix=/usr/bin --fuser=root --fgroup=root --version=1.4 --release=1 pup
clean:
	rm -f *.o pup