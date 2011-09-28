# Top level makefile, the real shit is at src/Makefile

TARGETS=32bit noopt test

all:
	cd src && $(MAKE) $@

install: dummy
	cd src && $(MAKE) $@

clean:
	cd deps/hiredis && $(MAKE) $@
	cd src && $(MAKE) $@

$(TARGETS):
	cd src && $(MAKE) $@

dummy:
