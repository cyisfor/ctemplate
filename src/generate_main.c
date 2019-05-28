#include "generate.h"
#include "mystring.h"
#include "die.h"
#include <stdlib.h> // getenv
#include <libgen.h> // dirname
#include <error.h>
#include <errno.h>
#include <assert.h>

#define ensure0(expr) if(0 != (expr)) error(errno, errno, #expr)

int main(int argc, char *argv[])
{
	if(NULL != getenv("open")) {
		char* open = getenv("open");
		char* close = getenv("close");
		if(close == NULL) {
			die("You must specify both open= and close=");
		}
		generate_config.open = (string){
			.s = open,
			.l = strlen(open)
		};
		generate_config.close = (string){
			.s = close,
			.l = strlen(close)
		};
	} else if(NULL != getenv("close")) {
		die("You must also specify open= as well as close=");
	}
	generate_config.keep_space = NULL != getenv("KEEP_SPACE");
	if(argc == 3) {
		FILE* input = fopen(argv[1],"rt");
		assert(input);
		char* dest = argv[2];
		size_t destlen = strlen(dest);
		char tempname[destlen + LITSIZ(".temp")+1];
		memcpy(tempname, dest, destlen);
		memcpy(tempname+destlen,LITLEN(".temp"));
		tempname[destlen+LITSIZ(".temp")] = 0;
		FILE* output = fopen(tempname, "wt");
		assert(output);
		generate(output, input);
		ensure0(fclose(input));
		ensure0(fflush(output));
		ensure0(rename(tempname, dest));
	} else {
		generate(stdout, stdin);
	}
	return 0;
}
