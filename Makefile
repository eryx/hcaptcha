
CC=gcc

CFLAGS=-levent -lgd -lmemcached -O3 -g -Wall
OBJ=hcaptchad.o sds.o

hcaptchad: hcaptchad.c
	$(CC) -o hcaptchad hcaptchad.c sds.c $(CFLAGS)

clean: hcaptchad
	rm -f hcaptchad

install: hcaptchad
	install $(INSTALL_FLAGS) -m 4755 -o root hcaptchad $(DESTDIR)/usr/bin
	

