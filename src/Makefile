
CC=gcc
CFLAGS=-levent -lgd -lmemcached -I../deps/hiredis -O3 -g -Wall

CCCOLOR="\033[34m"
LINKCOLOR="\033[34;1m"
SRCCOLOR="\033[33m"
BINCOLOR="\033[37;1m"
MAKECOLOR="\033[32;1m"
ENDCOLOR="\033[0m"

ifndef V
QUIET_CC = @printf '    %b %b\n' $(CCCOLOR)CC$(ENDCOLOR) $(SRCCOLOR)$@$(ENDCOLOR);
QUIET_LINK = @printf '    %b %b\n' $(LINKCOLOR)LINK$(ENDCOLOR) $(BINCOLOR)$@$(ENDCOLOR);
endif


CCOPT= $(CFLAGS) $(ARCH) $(PROF)

PREFIX= /usr/local
INSTALL_BIN= $(PREFIX)/bin
INSTALL= cp -p

OBJ=hcaptchad.o ../deps/hiredis/sds.o
PRGNAME=hcaptchad

all: dependencies hcaptchad 
	@echo ""
	@echo "hcaptchad build complete ;)"
	@echo ""

# Deps (use make dep to generate this)
hcaptchad.o: hcaptchad.c

dependencies:
	@printf '%b %b\n' $(MAKECOLOR)MAKE$(ENDCOLOR) $(BINCOLOR)hiredis$(ENDCOLOR)
	cd ../deps/hiredis && $(MAKE) 

hcaptchad: dependencies $(OBJ)
	$(QUIET_LINK)$(CC) -o $(PRGNAME) $(CCOPT) $(DEBUG) $(OBJ) $(CCLINK)

.c.o:
	$(CC) -c $(CFLAGS) $(OBJARCH) $(DEBUG) $(COMPILE_TIME) $<
	
clean:
	rm -rf $(PRGNAME) *.o *.gcda *.gcno *.gcov

dep:
	$(CC) -MM *.c -I ../deps/hiredis 

install: all
	mkdir -p $(INSTALL_BIN)
	$(INSTALL) $(PRGNAME) $(INSTALL_BIN)

