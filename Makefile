VPATH=src

CFLAGS+=-O2 -ggdb

O=$(patsubst %,o/%.o,$(N))
EXE=$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^ $(LDLIBS)

all: example

N=generate
generate: $(O)
	$(EXE)

N=example
example: $(O)
	$(EXE)

o/example.o: o/test.template.html.c

o/example.o: CFLAGS+=-I.

o/%.c: % generate
	./generate <$< >$@.temp
	mv $@.temp $@

o/%.o: %.c | o
	$(CC) $(CFLAGS) -c -o $@ $<

o:
	mkdir $@
