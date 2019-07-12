#include "mystring.h"
#include <stdio.h>

struct generate_options {
	string tag;
};

void generate(FILE* out, string in, struct generate_options opt);
