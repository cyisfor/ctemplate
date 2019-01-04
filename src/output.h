/* suggested output setup
	 note if you have a socket, you can fdopen it.
	 you could also resize your own buffer, but why?
*/

#include <stdio.h>
#include <stdlib.h> // size_t, NULL
#include <string.h> // strlen

// assign this somewhere
FILE* out = NULL;

#define output_literal(lit) fwrite(lit,sizeof(lit)-1,1,out)
#define output_buf(s,len) fwrite(s,len,1,out)
#define output_fmt(fmt, ...) fprintf(out,fmt, ## __VA_ARGS__)
#define output_str(str) output_buf(str.s, str.l);
#include "mystring.h"
