#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h> // abort

void die(const char* fmt, ...) {
	va_list arg;
	va_start(arg, fmt);
	vfprintf(stderr, fmt, arg);
	va_end(arg);
	fputc('\n',stderr);
	abort();
}
