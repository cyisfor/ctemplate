all: build/Makefile
	$(MAKE) -C build && $(MAKE) -C build install

build/Makefile: configure | build
	cd build && ../configure --prefix=$(PWD) --bindir=$(PWD)

configure: configure.ac Makefile.in config.h.in
	autoconf

config.h.in:
	autoheader

Makefile.in: Makefile.am aclocal.m4
	automake --add-missing

aclocal.m4: m4/libtool.m4
	aclocal -I m4

m4/libtool.m4:
	libtoolize --automake

build:
	mkdir $@

.PHONY: all
