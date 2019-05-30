.|C
#include <stdio.h> // 

#include "output.h"
int main(void) {
	int foo = 5;
	out = stdout;
	fputs("\\n**** This is compile time, while making the runtime code.\\n\\n", stderr);
	|.#include <stdio.h>
			
		int main(void) {
			puts("runtime out of template goes here.");
			int bar = .|c "%d", foo|.;
			printf("in runtime, bar is %d\n", bar);
			printf("in compile time, foo is .|c "%d", foo|.\n");
			int i;
			for(i=0;i<bar;++i) {
				puts("foo");
			}
			.|C
					int i;
			for(i=0;i<foo;++i) {
				|. puts("bar");
			.|C;
			}
			|.
		}
		.|C
			}
|.
