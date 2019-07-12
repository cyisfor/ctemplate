#include "c_generate.h"
#include "mmapfile.h"
int main(int argc, char *argv[])
{
	struct generate_options opts = {};
	if(NULL != getenv("tag")) {
		opts.tag = strlenstr(getenv("tag"));
    }
	assert(argc == 3);
	string input;
	input.base = mmapfile(argv[1], &input.len);
	assert(input.base);
	char* dest = argv[2];
	size_t destlen = strlen(dest);
	char tempname[destlen + LITSIZ(".temp")+1];
	memcpy(tempname, dest, destlen);
	memcpy(tempname+destlen,LITLEN(".temp"));
	tempname[destlen+LITSIZ(".temp")] = 0;
	FILE* output = fopen(tempname, "wt");
	assert(output);
	generate(output, input);
	ensure0(munmap((void*)input.base, input.len));
	ensure0(fflush(output));
	ensure0(fclose(output));
	ensure0(rename(tempname, dest));
    return 0;
}
