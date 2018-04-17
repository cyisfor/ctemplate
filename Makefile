all: build/Makefile
	$(MAKE) -C build

build/Makefile: Makefile.in configure | build
	cd build && ../configure

configure: configure.ac
	autoconf

Makefile.in: Makefile.am m4/libtool.m4
	automake

m4/libtool.m4:
	libtoolize --automake

build:
	mkdir $@

.PHONY: all
