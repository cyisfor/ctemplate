#include "mystring.h"

#include <stdio.h>
#include <stdarg.h>

size_t string_va_args_size(string sentinel, ...) {
	va_list args;
	va_start(args, sentinel);
	size_t num = 0;
	for(;;) {
		string next = va_arg(args, string);
		if(next.len == sentinel.len && next.base == sentinel.base) {
			return num;
		}
		++num;
	}
}

void output_function(FILE* out, size_t nargs, ...) {
	va_list args;
	va_start(args, nargs);
	size_t i;
	for(i=0;i<nargs;++i) {
		string arg = va_arg(args, string);
		fwrite(arg.base, arg.len, 1, out);
	}
}

string sentinel = { .base = (void*)0x1234, .len = 0x1337 };
			   
#define output_stuff(narg, ...) { output_function(stdout, narg, __VA_ARGS__); }

int main(int argc, char *argv[])
{
	char buf1[20];
	output_stuff(3,
				 LITSTR("this is a "),
				 ({
					 int i = 0;
					 int sum = 0;
					 for(i=0;i<10;++i) {
						 sum += i;
					 }
			
					 (string){buf1,snprintf(buf1,20,"%d",sum)};
				 })
				 ,LITSTR(" test"));
	putchar('\n');
    
    return 0;
}
