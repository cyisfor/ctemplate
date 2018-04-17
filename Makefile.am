ACLOCAL_AMFLAGS = -Im4

AM_CFLAGS = -I$(srcdir)/src

#CFLAGS+=-O2

bin_PROGRAMS = generate example
lib_LTLIBRARIES = libgenerate.la

BUILT_SOURCES=o/test.template.html.c

o/%.c: $(srcdir)/template/% ./generate | o
	./generate <$< >$@.temp
	mv $@.temp $@

example_SOURCES = src/example.c o/test.template.html.c

generate_SOURCES = src/generate_main.c

libgenerate_la_SOURCES = src/generate.c

o:
	mkdir $@
