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


typedef struct string {
	char* s;
	size_t l;
} string;

typedef struct generate_context {
	FILE* in;
	FILE* out;
} generate_context;

typedef generate_context context; // derp

struct generate_config {
	bool keep_space;
};

struct generate_config generate_config = {
	.keep_space = false
};

// in/out to be defined
#define PUTS(s,len) fwrite(s,len,1,out)
#define PUTLIT(lit) PUTS(lit,sizeof(lit)-1)
#define GETC(fmt, ...) safefgetc(in, fmt, ## __VA_ARGS__)

static
void generate(FILE* out, FILE* in) {
	bool noass = true;
	char namebuf[0x100];
	string name = {
		.s = namebuf,
		.l = 0
	};

	enum kinds kind;

	char* curlit = malloc(16); // because 0 * 1.5 == 0 oops
	size_t clpos = 0;
	size_t clsize = 16;

	/* utility functions */

	void put_quoted(const char* s, size_t l) {
		size_t i;
		size_t lastnl = 0;
		for(i=0;i<l;++i) {
			switch(s[i]) {
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
				if(i == l - 1 || (i - lastnl) < 4) {
					// ...except at the end, or if there was a newline recently
					PUTLIT("\\n");
				} else {
					PUTLIT("\\n\"\n\t\"");
				}
				lastnl = i;
				break;
			default:
				fputc(s[i],out);
				break;
			};
		}
	}
	
	void add(char c) {
		if(clpos == clsize) {
			clsize = (clsize*3)>>1;
			curlit = realloc(curlit,clsize);
		}
		curlit[clpos++] = c;
	}

	void commit_curlit() {
		if(clpos > 0) {
			PUTLIT("output_literal(\"");
			put_quoted(curlit,clpos);
			PUTLIT("\");\n");
			clpos = 0;
		}
	}

	char c;

	void ADVANCE(void) {
		c = fgetc(in);
	}

	void EXPECT(const char* msg) {
		ADVANCE();
		if(feof(in)) {
			error(msg);
		}
	}

	/* state machine */

	bool noass;
	
	void process_code(enum kinds kind) {
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

		// output the actual code
		for(;;) {
			// XXX: technically could just assume EOF means ?>
			char c = GETC("expected ?> before EOF to end code");
			switch(c) {
			case '\\':
				c = GETC("expected character after backslash");
				switch(c) {
				case '?':
					// escaped ?
					fputc('?',);
					continue;
				default: // this also gets \\
					fputc('\\',);
					fputc(c,);
					continue;
				};
			case '?':
				c = GET("must end code in ?>");
				if(c == '>') {
					goto FINISH_CODE;
				} else {
					// oops
					fputc('?',);
					fputc(c, );
					continue;
				}
			default:
				fputc(c, );
				continue;
			};
		}
		
FINISH_CODE:
		switch(kind) {
		case CODE:
			PUTLIT("\n");
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
		ADVANCE();
		if(feof(in)) {
		} else if(c == '\n') {
			ADVANCE();
		}
	}
	
	void process_string() {
		EXPECT("expected l, s, or whitespace after <?c");
		switch(c) {
		case ' ':
		case '\t':
		case '\n':
			return process_code(FMT);
		case 'l':
			return process_code(LIT);
		case 's':
			EXPECT("expected l or whitespace after <?cs");
			switch(c) {
			case ' ':
			case '\t':
			case '\n':
				return process_code(LITWLEN);
			case 'l':
				EXPECT("whitespace after <?csl");
				if(!isspace(c))
					error("whitespace should follow <?csl");
				return process_code(ZSTR);
			default:
				// oops
				add('<');
				add('?');
				add('c');
				add('s');
				add(c);
				return;
			};
			break;
		case 'S':
			EXPECT("expected character, whitespace, 1 or ( after <?cS");
			
			noass = true;
			switch(c) {
			case ' ':
			case '\t':
			case '\n':
				// default name is S
				noass = false;
				name.s[0] = 'S';
				name.l = 1;
				// process_thingy(STRING, noass)
				return process_code(STRING);
			case '1':
				// XXX: this is mostly useless...
				noass = true;
				EXPECT("expected whitespace after <?cS1");
				assert(isspace(c));
				return process_code(STRING);
			case '(':
				name.l = 0;
				for(;;) {
					EXPECT("expected ) after <?cS(");
					if(c == ')') break;
					name.s[name.l++] = c;
					assert(name.l < 0x100);
				}
				return process_code(STRING);
			default: 
				name.s[0] = c;
				name.l = 1;
				EXPECT("expected whitespace after <?cS?");
				assert(isspace(c));
				return process_code(STRING);
			};
			break;
		default:
			// oops
			add('<');
			add('?');
			add(c);
		};
	}

	void process_literal(void) {
		// assumes we already ADVANCE at least once
		for(;;) {
			if(feof(in)) break;
			switch(c) {
			case '\\': {
				ADVANCE();
				if(feof(in)) {
					// warn(trailing backslash)
					add('\\');
					commit_curlit();
					return;
				}
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
				ADVANCE();
				if(feof(in)) {
					add('<');
					commit_curlit();
					return;
				}
				if(c != '?') {
					// oops
					add('<');
					add(c);
					ADVANCE();
					continue;
				}
				EXPECT("expected stuff after <?");
				// process_directive
				switch(c) {
				case 'C':
					process_code(CODE);
					break;
				case 'c':
					process_string();
					break;
				default:
					//oops
					add('<');
					add('?');
					add(c);
					ADVANCE();
					continue;
				};
			default:
				add(c);
				ADVANCE();
			};
		}
		return commit_curlit();
	}

	ADVANCE();

	if(!generate_config.keep_space) {
		while(isspace(c)) ADVANCE();
	}

	return process_literal();
}
