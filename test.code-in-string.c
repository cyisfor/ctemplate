#include <stdio.h>

int main(int argc, char *argv[])
{
	const char* message = "this is a "
		({
			char buf[20];
			int i = 0;
			int sum = 0;
			for(i=0;i<10;++i) {
				sum += i;
			}
			snprintf(buf,20,"%d",sum);
			buf
				})
		" test";
	puts(message);
    
    return 0;
}
