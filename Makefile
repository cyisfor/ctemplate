VPATH=src

#CFLAGS+=-O2
CFLAGS+=-ggdb
CFLAGS+=$(INC)

O=$(patsubst %,o/%.o,$(N))
EXE=$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^ $(LDLIBS)

all: example
	./example

N=generate_main
generate: $(O) libgenerate.a
	$(EXE)

N=generate
libgenerate.a: $(O)
	ar crs $@ $^

N=example
example: $(O)
	$(EXE)

o/example.o: o/test.template.html.c

o/example.o: INC+=-I.
# since generate depends on example.o, the extra CFLAGS end up there.
generate: INC:=

o/%.c: % generate
	./generate <$< >$@.temp
	mv $@.temp $@

o/%.o: %.c | o
	$(CC) $(CFLAGS) -c -o $@ $<

o:
	mkdir $@

.PHONY: all
