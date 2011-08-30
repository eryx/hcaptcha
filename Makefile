
CC=gcc

CFLAGS=-levent -lgd -lmemcached -O3 -g -Wall

xml: hcaptchad
	$(CC) -o hcaptchad hcaptchad.c $(CFLAGS)

clean: hcaptchad
	rm -f hcaptchad

install: hcaptchad
	install $(INSTALL_FLAGS) -m 4755 -o root hcaptchad $(DESTDIR)/usr/bin
	

