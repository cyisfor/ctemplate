#include <stdio.h>
#include <stdbool.h>

struct generate_config {
	bool keep_space;
};

extern struct generate_config generate_config;

void generate(FILE* out, FILE* in);
