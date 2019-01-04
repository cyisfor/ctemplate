#include "generate.h"
#include "mystring.h"
#include <stdlib.h> // getenv
#include <libgen.h> // dirname
#include <error.h>
#include <errno.h>
#include <assert.h>

#define ensure0(expr) if(0 != (expr)) error(errno, errno, #expr)

int main(int argc, char *argv[])
{
	generate_config.keep_space = NULL != getenv("KEEP_SPACE");
	if(argc == 3) {
		FILE* input = fopen(argv[1],"rt");
		size_t dlen;
		assert(input);
		char* dest = argv[2];
		int destlen = strlen(dest);
		char tempname[dlen + LITSIZ(".temp")+1];
		memcpy(tempname, dest, destlen);
		memcpy(tempname+destlen,LITLEN(".temp"));
		tempname[destlen+LITSIZ(".temp")] = 0;
		FILE* output = fopen(tempname, "wt");
		assert(output);
		generate(input, output);
		ensure0(fclose(input));
		ensure0(fflush(output));
		ensure0(rename(tempname, dest));
	} else {
		generate(stdout, stdin);
	}
	return 0;
}
