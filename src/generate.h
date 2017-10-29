#include <stdio.h>
#include <stdbool.h>

struct generate_config {
	bool keep_space;
};

extern struct generate_config generate_config;

typedef struct generate_context* generate_context;

generate_context generate_init(void);
void generate_free(generate_context* pctx);
void generate(generate_context ctx, FILE* out, FILE* in);
