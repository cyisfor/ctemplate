#include <stdio.h>

#include <stdarg.h>
#include <stdlib.h> // malloc, NULL
#include <stdbool.h>
#include <ctype.h> // isspaceva

#define assert(a) if(!(a)) abort();

enum kinds { EHUNNO, CODE, FMT, LIT, LITWLEN, ZSTR, STRING };

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

struct generate_config {
	bool keep_space;
};

struct generate_config generate_config = {
	.keep_space = false
};

// in/out to be defined
#define PUTS(s,len) fwrite(s,len,1,out)
#define PUTLIT(lit) PUTS(lit,sizeof(lit)-1)

void generate(FILE* out, FILE* in) {
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

	struct parser {
		char cur;
		char next;
	} c;

	/* note: ALWAYS advance after you've dealt with the current value of c */
	// return the old value, so we can switch on it, but assume advanced
	char ADVANCE(void) {
		// lookahead eh
		c.cur = c.next;
		c.next = fgetc(in);
		return c.cur;
	}

	char EXPECT(const char* msg) {
		ADVANCE();
		if(feof(in)) {
			error(msg);
		}
		return c.cur;
	}

	/* state machine */

	bool noass;
	char namebuf[0x100];
	string name = {
		.s = namebuf,
		.l = 0
	};
	
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
			switch(EXPECT("expected ?> before EOF to end code")) {
			case '\\':
				switch(EXPECT("expected character after backslash")) {
				case '?':
					// escaped ?
					fputc('?', out);
					ADVANCE();
					continue;
				default: // this also gets \\
					fputc('\\', out);
					fputc(c.cur, out);
					continue;
				};
			case '?':
				if(c.next == '>') {
					goto FINISH_CODE;
				} else {
					// oops
					fputc('?', out);
					fputc(c.next, out);
					EXPECT("must end code in ?>");
					continue;
				}
			default:
				fputc(c.cur, out);
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
		} else if(c.cur == '\n') {
			ADVANCE();
		}
	}
	
	void process_string() {
		switch(EXPECT("expected l, s, or whitespace after <?c")) {
		case ' ':
		case '\t':
		case '\n':
			return process_code(FMT);
		case 'l':
			return process_code(LIT);
		case 's':
			switch(EXPECT("expected l or whitespace after <?cs")) {
			case ' ':
			case '\t':
			case '\n':
				return process_code(LITWLEN);
			case 'l':
				EXPECT("whitespace after <?csl");
				if(!isspace(c.cur))
					error("whitespace should follow <?csl");
				return process_code(ZSTR);
			default:
				// oops
				add('<');
				add('?');
				add('c');
				add('s');
				add(c.cur);
				return;
			};
			break;
		case 'S':
			noass = true;
			switch(EXPECT("expected character, whitespace, 1 or ( after <?cS")) {
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
				assert(isspace(EXPECT("expected whitespace after <?cS1")));
				return process_code(STRING);
			case '(':
				name.l = 0;
				for(;;) {
					EXPECT("expected ) after <?cS(");
					if(c.cur == ')') break;
					name.s[name.l++] = c.cur;
					assert(name.l < 0x100);
				}
				return process_code(STRING);
			default: 
				name.s[0] = c.cur;
				name.l = 1;
				assert(isspace(c.next));
				EXPECT("expected whitespace after <?cS?");
				return process_code(STRING);
			};
			break;
		default:
			// oops
			add('<');
			add('?');
			add(c.cur);
		};
	}

	void process_literal(void) {
		// assumes we already ADVANCE at least once
		for(;;) {
			if(feof(in)) break;
			switch(ADVANCE()) {
			case '\\': {
				if(feof(in)) {
					// warn(trailing backslash)
					add('\\');
					return commit_curlit();
				}
				switch(ADVANCE()) {
				case '<':
					add('<');
					return;
				case '\\':
					add('\\');
					return;					
				default:
					add('\\');
					add(c.cur);
					return;
				};
			}
			case '<':
				if(feof(in)) {
					add('<');
					commit_curlit();
					return;
				}
				if(ADVANCE() != '?') {
					// oops
					add('<');
					add(c.cur);
					continue;
				}
				// process_directive
				switch(EXPECT("expected stuff after <?")) {
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
					add(c.cur);
					continue;
				};
			default:
				add(c.cur);
			};
		}
		return commit_curlit();
	}

	ADVANCE();

	if(!generate_config.keep_space) {
		while(isspace(c.cur)) ADVANCE();
	}

	return process_literal();
}
