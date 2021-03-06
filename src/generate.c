#include "generate.h"
#include "die.h"

#include <stdarg.h>
#include <stdlib.h> // malloc, NULL
#include <ctype.h> // isspaceva
#include <error.h>
#include <errno.h>
#include <string.h> // memcpy
#include <stdint.h> // intptr_t

#define alwaysassert(a) if(!(a)) abort();

enum kinds { EHUNNO, CODE, FMT, LIT, LITWLEN, CSTRING, STRING };

struct generate_config generate_config = {
	.keep_space = false,
	.open = (string){ .base = "<?", .len = 2},
	.close = (string){ .base = "?>", .len = 2},
};

#define G_O generate_config.open
#define G_C generate_config.close 

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
					PUTLIT("\\n\" \\\n\t\"");
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

	void adds(string s) {
		if(clpos + s.len > clsize) {
			clsize = ((clpos + s.len)/1024+1)*1024;
			curlit = realloc(curlit, clsize);
		}
		memcpy(curlit + clpos, s.base, s.len);
		clpos += s.len;
	}

	void commit_curlit(bool done) {
		if(clpos > 0) {
			PUTLIT("output_literal(\"");
			put_quoted(curlit,clpos);
			PUTLIT("\");\n");
			clpos = 0;
			if(done)
				free(curlit);
		}
	}

	struct parser {
		char cur;
		char next;
	} c = {};

	/* note: ALWAYS advance after you've dealt with the current value of c */
	// return the old value, so we can switch on it, but assume advanced
	char ADVANCE(void) {
		// lookahead eh
		c.cur = c.next;
		c.next = fgetc(in);
		if(c.next == -1) {
			if(!feof(in)) {
				error(errno,errno,"fgetc error");
			}
		}
		return c.cur;
	}

	char EXPECT(const char* fmt, ...) {
		ADVANCE();
		if(feof(in)) {
			void* args = __builtin_apply_args();
			printf("uhh %ld\n", (intptr_t)args - (intptr_t)&fmt);
			__builtin_return(__builtin_apply((void*)die,
																			 args,
																			 0x400));
		}
		return c.cur;
	}

	bool advance_str(string str) {
		int i;
		for(i=1;i<str.len;++i) {
			char c = ADVANCE();
			if(c != str.base[i]) {
				// oops, not an opening thingy
				const string pfx = {.base = str.base, .len = i};
				adds(pfx);
				add(c);
				return false;
			}
		}
		return true;
	}


	/* state machine */

	bool noass;
	char namebuf[0x100];
	bstring name = {
		.base = namebuf,
		.len = 0
	};
	
	void process_code(enum kinds kind) {
		commit_curlit(false);
			
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
		case CSTRING:
			PUTLIT("{ const char* s = ");
		case STRING:
			if(!noass) {
				PUTLIT("{ string ");
				PUTS(name.base,name.len);
				PUTLIT("; ");
			} 
		};

		// since curlit isn't currently being used, let's temporarily store the code in it
		// so we can duplicate it as in <?cS ... ?>
		// don't commit_curlit though! that'll add a spurious output_literal(...) wrapper

		// output the actual code
		for(;;) {
			// XXX: technically could just assume EOF means ?>
			char cc = EXPECT("expected %.* before EOF to end code", G_C.len, G_C.base);
			if(cc == '\\') {
				cc = EXPECT("expected character after backslash");
				if(cc == G_C.base[0]) {
					// escaped ?
					add(G_C.base[0]);
					ADVANCE();
				} else {
					// this also gets \\
					add('\\');
					add(cc);
				}
			} else if(cc == G_C.base[0]) {
				if (advance_str(G_C) == true)
					break;
			} else {
				add(cc);
			}
		}

		void put_code(void) {
			alwaysassert(clpos > 0);
			if(curlit[clpos-1] == '\n') {
				// special case, ignore final \n directly before ?>
				--clpos;
			}
			fwrite(curlit,clpos,1,out);
		}
		void commit_code(void) {
			put_code();
			clpos = 0;
		}
		
FINISH_CODE:
		switch(kind) {
		case CODE:
			commit_code();
			PUTLIT("\n");
			break;
		case CSTRING:
			commit_code();
			PUTLIT("; output_buf(s,strlen(s)); }\n");
			break;
		case STRING:
			if(noass) {
				PUTLIT("output_buf((");
				put_code();
				PUTLIT(").base, (");
				commit_code();
				PUTLIT(").len);\n");
			} else {
				PUTLIT("; }); output_buf(S.base, S.len); }\n");
			}
			break;
		default:
			commit_code();
			PUTLIT(");\n");
		};
		// check for a newline following ?>
		if(feof(in)) {
		} else if(c.cur == '\n') {
			ADVANCE();
		}
	}
	
	void process_string() {
		switch(EXPECT("expected l, s, or whitespace after %.*sc", G_O.len, G_O.base)) {
		case ' ':
		case '\t':
		case '\n':
			return process_code(FMT);
		case 'l':
			return process_code(LIT);
		case 's':
			switch(EXPECT("expected l or whitespace after %.*scs", G_O.len, G_O.base)) {
			case ' ':
			case '\t':
			case '\n':
				return process_code(LITWLEN);
			case 'l':
				EXPECT("whitespace after %.*scsl", G_O.len, G_O.base);
				if(!isspace(c.cur))
					die("whitespace should follow %.*csl", G_O.len, G_O.base);
				return process_code(CSTRING);
			default:
				// oops
				adds(G_O);
				add('c');
				add('s');
				add(c.cur);
				return;
			};
			break;
		case 'S':
			noass = true;
			switch(EXPECT("expected character, whitespace, 1 or ( after %.*scS", G_O.len, G_O.base)) {
			case ' ':
			case '\t':
			case '\n':
				// just duplicate the phrase for ().base and ().len
				noass = true;
				return process_code(STRING);
			case '(':
				name.len = 0;
				for(;;) {
					EXPECT("expected ) after %.*scS(", G_O.len, G_O.base);
					if(c.cur == ')') break;
					name.base[name.len++] = c.cur;
					alwaysassert(name.len < 0x100);
				}
				return process_code(STRING);
			default: 
				name.base[0] = c.cur;
				name.len = 1;
				alwaysassert(isspace(c.next));
				EXPECT("expected whitespace after %.*scS?", G_O.len, G_O.base);
				return process_code(STRING);
			};
			break;
		default:
			// oops
			adds(G_O);
			add(c.cur);
		};
	}

	void process_literal(void) {
		// assumes we already ADVANCE at least once
		for(;;) {
			if(feof(in)) {
				// strip tail
				if(clpos) {
					while(isspace(curlit[clpos-1])) {
						if(--clpos == 0) break;
					}
				}	
				break;
			}
			char cc = ADVANCE();
			if(cc == '\\') {
				if(feof(in)) {
					// warn(trailing backslash)
					add('\\');
					return commit_curlit(true);
				}
				cc = ADVANCE();
				if(cc == G_O.base[0]) {
					add(G_O.base[0]);
				} else if(cc == '\\') {
					add('\\');
				} else {
					add('\\');
					add(cc);
				}
			} else if(cc == G_O.base[0]) {
				if(feof(in)) {
					add(cc);
					commit_curlit(true);
					return;
				}
				if(advance_str(G_O) == false) continue;
				// process_directive
				switch(EXPECT("expected stuff after %.*s", G_O.len, G_O.base)) {
				case 'C':
					process_code(CODE);
					break;
				case 'c':
					process_string();
					break;
				default:
					//oops
					adds(G_O);
					add(c.cur);
					continue;
				};
			} else {
				add(c.cur);
			}
		}
		return commit_curlit(true);
	}

	ADVANCE();

	if(!generate_config.keep_space) {
		while(isspace(c.next)) ADVANCE();
	}

	return process_literal();
}
