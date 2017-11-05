#include "generate.h"
#include <stdlib.h> // getenv

int main(int argc, char *argv[])
{
	generate_config.keep_space = NULL != getenv("KEEP_SPACE");
	generate(stdout, stdin);
	return 0;
}
