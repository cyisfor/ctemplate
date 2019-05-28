#ifndef GENERATE_H
#define GENERATE_H

#include "mystring.h"

#include <stdio.h>
#include <stdbool.h>
#include <stddef.h> // size_t

struct generate_config {
	bool keep_space;
	const string open;
	const string close;
};

extern struct generate_config generate_config;

void generate(FILE* out, FILE* in);


#endif /* GENERATE_H */
