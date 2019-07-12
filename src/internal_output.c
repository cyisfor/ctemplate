#include "internal_output.h"

#define PUTLIT(lit) fwrite(lit, sizeof(lit)-1, 1, out)

void output_escaped(FILE* out, string s) {
	size_t i;
	size_t lastnl = 0;
	for(i=0;i<s.len;++i) {
		switch(s.base[i]) {
		case '\"':
			fputc('\\',out);
			fputc('"',out);
			break;
		case '\\':
			fputc('\\',out);
			fputc('\\',out);
			break;
		case '\n':
			// this logically divides the string up into "..." things, which
			// the compiler will concatenate
			if(i == s.len - 1 || (i - lastnl) < 4) {
				// ...except at the end, or if there was a newline recently
				PUTLIT("\\n");
			} else {
				PUTLIT("\\n\" \\\n\t\"");
			}
			lastnl = i;
			break;
		default:
			fputc(s.base[i],out);
			break;
		};
	}
}
