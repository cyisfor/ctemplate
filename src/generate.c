#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h> // malloc, NULL
#include <stdbool.h>
#include <ctype.h> // isspaceva

#define assert(a) if(!(a)) abort();


enum kinds { EHUNNO, CODE, FMT, LIT, LITWLEN, ZSTR, STRING };

static
char safefgetc(FILE* fp, const char* fmt, ...) {
	char ret = fgetc(fp);
	if(feof(fp)) {
		fputs("EOF on getc\n",stderr);
		va_list arg;
		va_start(arg, fmt);
		vfprintf(stderr, fmt, arg);
		va_end(arg);
		fputc('\n',stderr);
		abort();
	}
	return ret;
}

static
void error(const char* fmt, ...) {
	va_list arg;
	va_start(arg, fmt);
	vfprintf(stderr, fmt, arg);
	va_end(arg);
	fputc('\n',stderr);
	abort();
}

#define PUTS(s,len) fwrite(s,len,1,stdout)
#define PUTLIT(lit) PUTS(lit,sizeof(lit)-1)

typedef struct string {
	char* s;
	size_t l;
} string;

static
void put_quoted(const char* s, size_t l) {
	size_t i;
	size_t lastnl = 0;
	for(i=0;i<l;++i) {
		switch(s[i]) {
		case '\"':
			fputc('\\',stdout);
			fputc('"',stdout);
			break;
		case '\\':
			fputc('\\',stdout);
			fputc('\\',stdout);
			break;
		case '\n':
			// this logically divides the string up into "..." things, which
			// the compiler will concatenate
			if(i == l - 1 || (i - lastnl) < 4) {
				// ...except at the end, or if there was a newline recently
				PUTLIT("\\n");
			} else {
				PUTLIT("\\n\"\n\t\"");
			}
			lastnl = i;
			break;
		default:
			fputc(s[i],stdout);
			break;
		};
	}
}


#define GETC(fmt, ...) safefgetc(stdin, fmt, ## __VA_ARGS__)

enum kinds kind = EHUNNO;
char namebuf[0x100]; // eh, whatever
bool noass; // no assignment
/* ass: string s = { ... }; output_buf(...);
	 noass: string s; ... output_buf(...);
*/
string name = {.s = namebuf, .l = 0}; // for <?cS(name)...?>
char* curlit = NULL;
size_t clpos = 0;
size_t clsize = 0;
void add(char c) {
	if(clpos == clsize) {
		clsize = (clsize*3)>>1;
		curlit = realloc(curlit,clsize);
	}
	curlit[clpos++] = c;
}
void commit_curlit(void) {
	if(clpos > 0) {
		PUTLIT("output_literal(\"");
		put_quoted(curlit,clpos);
		PUTLIT("\");\n");
		clpos = 0;
	}
}

// return true when done
bool process_code(char c) {
	switch(c) {
	case '\\':
		c = fgetc(stdin);
		if(feof(stdin)) return true;
		switch(c) {
		case '?':
			// escaped ?
			fputc('?',stdout);
			return false;
		default: // this also gets \\
			fputc('\\',stdout);
			fputc(c,stdout);
			return false;
		};
	case '?':
		c = fgetc(stdin);
		if(feof(stdin)) return true;
		if(c == '>') {
			return true;
		} else {
			// oops
			fputc('?',stdout);
			fputc(c, stdout);
			return false;
		}
	default:
		fputc(c, stdout);
		return false;
	};
}

bool process(char c) {
	switch(c) {
	case '\\': {
		c = fgetc(stdin);
		if(feof(stdin)) return true;
		switch(c) {
		case '<':
			add('<');
			return false;
		default:
			add('\\');
			add(c);
			return false;
		};
	}
	case '<':
		c = fgetc(stdin);
		if(feof(stdin)) return true;
		if(c != '?') {
			// oops
			add('<');
			add(c);
			return false;
		}
		c = GETC("expected ? after <");
		kind = EHUNNO;
		switch(c) {
		case 'C':
			kind = CODE;
			break;
		case 'c':
			c = GETC("expected l, s, or whitespace after <?c");
			switch(c) {
			case ' ':
			case '\t':
			case '\n':
				kind = FMT;
				break;
			case 'l':
				kind = LIT;
				break;
			case 's':
				c = GETC("expected l or whitespace after <?cs");
				switch(c) {
				case ' ':
				case '\t':
				case '\n':
					kind = LITWLEN;
					break;
				case 'l':
					kind = ZSTR;
								
					if(!isspace(GETC("expected whitespace after <?csl")))
						error("whitespace should follow kind clause");
					break;
				};
				break;
			case 'S':
				kind = STRING;
				c = GETC("expected whitespace or ( after <?cS");
				noass = true;
				switch(c) {
				case ' ':
				case '\t':
				case '\n':
					// default name is S
					noass = false;
					name.s[0] = 'S';
					name.l = 1;
					break;
				case '(':
					name.l = 0;
					for(;;) {
						c = GETC("expected ) after <?C(");
						if(c == ')') break;
						name.s[name.l++] = c;
						assert(name.l < 0x100);
					}
					break;
				default:
					c = GETC("expected whitespace after <?cS?");
					assert(isspace(c));
					name.s[0] = c;
					name.l = 1;
					break;
				};
				break;
			default:
				// oops
				add('<');
				add('?');
				add(c);
				return false;
			};
			break;
			
		};
			

		commit_curlit();
			
		switch(kind) {
		case CODE:
			break;
		case FMT:
			PUTLIT("output_fmt(");
			break;
		case LIT:
			PUTLIT("output_literal(");
			break;
		case LITWLEN:
			PUTLIT("output_buf(");
			break;
		case ZSTR:
			PUTLIT("{ const char* s = ");
		case STRING:
			PUTLIT("{ string ");
			if(noass) {
				PUTS(name.s,name.l);
				PUTLIT("; ");
			} else {
				PUTLIT("S = ({ ");
			};
		};

		for(;;) {
			// XXX: technically could just assume EOF means ?>
			char c = GETC("expected ?> before EOF to end code");
			if(process_code(c)) break;
		}

		switch(kind) {
		case CODE:
			break;
		case ZSTR:
			PUTLIT("; output_buf(s,strlen(s)); }\n");
			break;
		case STRING:
			if(noass) {
				PUTLIT("; output_buf(");
				PUTS(name.s,name.l);
				PUTLIT(".s, ");
				PUTS(name.s,name.l);
				PUTLIT(".l); }\n");
			} else {
				PUTLIT("; }); output_buf(S.s, S.l); }\n");
			}
			break;
		default:
			PUTLIT(");\n");
		};
		// check for a newline following ?>
		c = fgetc(stdin);
		if(c == '\n')
			break;
		else
			return process(c); // gcc can optimize tail recursion
	default:
		add(c);
	};
	return false;
}
int main(int argc, char *argv[])
{
	curlit = malloc(16);
	clsize = 16;
	
	// uhh...
	if(NULL==getenv("KEEP_SPACE")) {
		for(;;) {
			char c = fgetc(stdin);
			if(feof(stdin)) return true;
			if(!isspace(c)) {
				process(c);
				break;
			}
		}
	}
	for(;;) {
		char c = fgetc(stdin);
		if(feof(stdin)) break;
		if(process(c)) break;
	}
	commit_curlit();
	return 0;
}
