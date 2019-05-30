#include "output.h"

int main(int argc, char *argv[])
{
	string title = { .base = getenv("title") };
	if(title.base == NULL) {
		title = LITSTR("example");
	}	else {
		title.len = strlen(title.base);
	}

	int i;
	int derp = 10;

	out = stdout;
	
#include "test.template.html.c"
	
	return 0;
}
