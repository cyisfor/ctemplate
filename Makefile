all: build/Makefile
	$(MAKE) -C build && $(MAKE) -C build install

build/Makefile: Makefile.in configure | build
	cd build && ../configure --prefix=$(realpath ..)

configure: configure.ac
	autoconf

Makefile.in: Makefile.am m4/libtool.m4
	automake

m4/libtool.m4:
	libtoolize --automake

build:
	mkdir $@

.PHONY: all
