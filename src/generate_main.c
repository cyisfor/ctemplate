#include "generate.h"

int main(int argc, char *argv[])
{
	generate_config.keep_space = NULL != getenv("KEEP_SPACE");
	generate_context* ctx = generate_init();
	generate(ctx, stdout, stdin);
	generate_free(&ctx);
	return 0;
}
