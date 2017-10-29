#include "output.h"

int main(int argc, char *argv[])
{
	string title = { .s = getenv("title") };
	if(title.s == NULL) { title.s = "example"; title.l = sizeof("example")-1; }
	else { title.l = strlen(title.s); }

	int i;
	int derp = 10;

	out = stdout;
	
#include "o/test.template.html.c"
	
	return 0;
}
