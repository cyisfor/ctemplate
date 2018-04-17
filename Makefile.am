ACLOCAL_AMFLAGS = -Im4

AM_CFLAGS = -I$(srcdir)/src -fdiagnostics-color

bin_PROGRAMS = generate example
lib_LTLIBRARIES = libgenerate.la

BUILT_SOURCES=o/test.template.html.c

o/%.c: $(srcdir)/% ./generate | o
	./generate <$< >$@.temp
	mv $@.temp $@

example_SOURCES = src/example.c

generate_SOURCES = src/generate_main.c
generate_LDADD = libgenerate.la

libgenerate_la_SOURCES = src/generate.c

o:
	mkdir $@
