# Hiredis Makefile
# Copyright (C) 2010 Salvatore Sanfilippo <antirez at gmail dot com>
# This file is released under the BSD license, see the COPYING file

CFLAGS=-O3 -Wall
CCOPT= $(CFLAGS) $(CCLINK)

# Deps (use make dep to generate this)
sds.o: sds.c sds.h

.c.o:
	$(CC) -c $(CFLAGS) $(OBJARCH) $(COMPILE_TIME) $<

clean:
	rm -rf ${DYLIBNAME} ${STLIBNAME} $(BINS) hiredis-example* *.o *.gcda *.gcno *.gcov

dep:
	$(CC) -MM *.c

