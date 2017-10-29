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
	enum kinds kind;
	char namebuf[0x100]; // eh, whatever
	bool noass; // no assignment
/* ass: string s = { ... }; output_buf(...);
	 noass: string s; ... output_buf(...);
*/
	string name;
	char* curlit;
	size_t clpos;
	size_t clsize;
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

context* generate_init(void) {
	context* ctx = calloc(sizeof(struct generate_context),1);
	ctx->name.s = ctx->namebuf;
	ctx->curlit = malloc(16); // because 0 * 1.5 == 0 oops
	ctx->clsize = 16;
	return ctx;
}

void generate_free(context** pctx) {
	context* ctx = *pctx;
	*pctx = NULL;
	free(ctx);
}

#define PUTS(s,len) fwrite(s,len,1,ctx->out)
#define PUTLIT(lit) PUTS(lit,sizeof(lit)-1)

static
void put_quoted(context* ctx, const char* s, size_t l) {
	size_t i;
	size_t lastnl = 0;
	for(i=0;i<l;++i) {
		switch(s[i]) {
		case '\"':
			fputc('\\',ctx->out);
			fputc('"',ctx->out);
			break;
		case '\\':
			fputc('\\',ctx->out);
			fputc('\\',ctx->out);
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
			fputc(s[i],ctx->out);
			break;
		};
	}
}


#define GETC(fmt, ...) safefgetc(ctx->in, fmt, ## __VA_ARGS__)

static
void add(context* ctx, char c) {
	if(ctx->clpos == ctx->clsize) {
		ctx->clsize = (ctx->clsize*3)>>1;
		ctx->curlit = realloc(ctx->curlit,ctx->clsize);
	}
	ctx->curlit[ctx->clpos++] = c;
}

static
void commit_curlit(context* ctx) {
	if(ctx->clpos > 0) {
		PUTLIT("output_literal(\"");
		put_quoted(ctx, ctx->curlit,ctx->clpos);
		PUTLIT("\");\n");
		ctx->clpos = 0;
	}
}

// return true when done
static
bool process_code(context* ctx, char c) {
	switch(c) {
	case '\\':
		c = fgetc(ctx->in);
		if(feof(ctx->in)) return true;
		switch(c) {
		case '?':
			// escaped ?
			fputc('?',ctx->out);
			return false;
		default: // this also gets \\
			fputc('\\',ctx->out);
			fputc(c,ctx->out);
			return false;
		};
	case '?':
		c = fgetc(ctx->in);
		if(feof(ctx->in)) return true;
		if(c == '>') {
			return true;
		} else {
			// oops
			fputc('?',ctx->out);
			fputc(c, ctx->out);
			return false;
		}
	default:
		fputc(c, ctx->out);
		return false;
	};
}

static
bool process(context* ctx, char c) {
	switch(c) {
	case '\\': {
		c = fgetc(ctx->in);
		if(feof(ctx->in)) return true;
		switch(c) {
		case '<':
			add(ctx, '<');
			return false;
		default:
			add(ctx, '\\');
			add(ctx, c);
			return false;
		};
	}
	case '<':
		c = fgetc(ctx->in);
		if(feof(ctx->in)) return true;
		if(c != '?') {
			// oops
			add(ctx, '<');
			add(ctx, c);
			return false;
		}
		c = GETC("expected ? after <");
		ctx->kind = EHUNNO;
		switch(c) {
		case 'C':
			ctx->kind = CODE;
			break;
		case 'c':
			c = GETC("expected l, s, or whitespace after <?c");
			switch(c) {
			case ' ':
			case '\t':
			case '\n':
				ctx->kind = FMT;
				break;
			case 'l':
				ctx->kind = LIT;
				break;
			case 's':
				c = GETC("expected l or whitespace after <?cs");
				switch(c) {
				case ' ':
				case '\t':
				case '\n':
					ctx->kind = LITWLEN;
					break;
				case 'l':
					ctx->kind = ZSTR;
								
					if(!isspace(GETC("expected whitespace after <?csl")))
						error("whitespace should follow ctx->kind clause");
					break;
				};
				break;
			case 'S':
				ctx->kind = STRING;
				c = GETC("expected whitespace or ( after <?cS");
				ctx->noass = true;
				switch(c) {
				case ' ':
				case '\t':
				case '\n':
					// default name is S
					ctx->noass = false;
					ctx->name.s[0] = 'S';
					ctx->name.l = 1;
					break;
				case '(':
					ctx->name.l = 0;
					for(;;) {
						c = GETC("expected ) after <?C(");
						if(c == ')') break;
						ctx->name.s[ctx->name.l++] = c;
						assert(ctx->name.l < 0x100);
					}
					break;
				default:
					c = GETC("expected whitespace after <?cS?");
					assert(isspace(c));
					ctx->name.s[0] = c;
					ctx->name.l = 1;
					break;
				};
				break;
			default:
				// oops
				add(ctx, '<');
				add(ctx, '?');
				add(ctx, c);
				return false;
			};
			break;
			
		};
			

		commit_curlit(ctx);
			
		switch(ctx->kind) {
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
			if(ctx->noass) {
				PUTS(ctx->name.s,ctx->name.l);
				PUTLIT("; ");
			} else {
				PUTLIT("S = ({ ");
			};
		};

		for(;;) {
			// XXX: technically could just assume EOF means ?>
			char c = GETC("expected ?> before EOF to end code");
			if(process_code(ctx, c)) break;
		}

		switch(ctx->kind) {
		case CODE:
			PUTLIT("\n");
			break;
		case ZSTR:
			PUTLIT("; output_buf(s,strlen(s)); }\n");
			break;
		case STRING:
			if(ctx->noass) {
				PUTLIT("; output_buf(");
				PUTS(ctx->name.s,ctx->name.l);
				PUTLIT(".s, ");
				PUTS(ctx->name.s,ctx->name.l);
				PUTLIT(".l); }\n");
			} else {
				PUTLIT("; }); output_buf(S.s, S.l); }\n");
			}
			break;
		default:
			PUTLIT(");\n");
		};
		// check for a newline following ?>
		c = fgetc(ctx->in);
		if(feof(ctx->in) || c == '\n')
			break;
		else
			return process(ctx, c); // gcc can optimize tail recursion
	default:
		add(ctx, c);
	};
	return false;
}

void generate(context* ctx, FILE* out, FILE* in) {
	ctx->in = in;
	ctx->out = out;

	// uhh...
	if(generate_config.keep_space) {
		for(;;) {
			char c = fgetc(ctx->in);
			if(feof(ctx->in)) return;
			if(!isspace(c)) {
				process(ctx, c);
				break;
			}
		}
	}
	for(;;) {
		char c = fgetc(ctx->in);
		if(feof(ctx->in)) break;
		if(process(ctx, c)) break;
	}
	commit_curlit(ctx);
}


int main(int argc, char *argv[])
{
	generate_config.keep_space = NULL != getenv("KEEP_SPACE");
	generate_context* ctx = generate_init();
	generate(ctx, stdout, stdin);
	generate_free(&ctx);
	return 0;
}
