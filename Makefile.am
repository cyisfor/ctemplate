ACLOCAL_AMFLAGS = -Im4

AM_CFLAGS = -I$(srcdir)/src -fdiagnostics-color

bin_PROGRAMS = ctemplate
noinst_PROGRAMS = example
lib_LTLIBRARIES = libctemplate.la

BUILT_SOURCES=o/test.template.html.c

o/%.c: $(srcdir)/% ./ctemplate | o
	./ctemplate <$< >$@.temp
	mv $@.temp $@

example_SOURCES = src/example.c

ctemplate_SOURCES = src/generate_main.c
ctemplate_LDADD = libctemplate.la

libctemplate_la_SOURCES = src/generate.c

o:
	mkdir $@
