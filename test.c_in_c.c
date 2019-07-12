#include <stdio.h>

int main(int argc, char *argv[])
{
	puts("The code is "
		 ctemplate(l)
		 {
			 int foo = 42;
			 int bar = 23;
			 printf("derp %d", foo+bar);
		 });
    return 0;
}
