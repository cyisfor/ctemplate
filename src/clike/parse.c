enum paren_type {
	PARENTHESES,
	SQUARE_BRACKET,
	ANGLE_BRACKET,
	CURLY_BRACKET,
	UNDERSCORE,
	SINGLE_QUOTE,
	DOUBLE_QUOTE,
	/* XXX: need special logic so Q { OPEN; derp CLOSE; } becomes OPEN; derp CLOSE; instead of considering OPEN/CLOSE part of the parentetical */
	OPEN_CLOSE,
	START_END,
	CODE_TEMPLATE
};

/* as we output substituted output_literal statements, track the current line length, and auto-
 newline and outer-indent when it gets too long, INCLUDING within the quoted string!

 foo
   foo Q(1111111111111122222222222222233333333333333444444444444445555555555555) bar
 =>
 foo
   foo output_literal("11111111111111222222222222222333333333333334444444444444455"
   "55555555555") bar
*/
struct paren {
	int num;
	enum paren_type seq[];
};

struct parser {
	int outer_indent;
	int inner_indent;
};

/* search forward for next interesting thing
 - newline+tabulation
 - Q
 - open paren
 - close paren if existing open paren
 - paren is union, either a flag for common, or a string for custom
 */

enum token_type {
#define X(ident, val) TOKEN_ ## ident
#include "for_tokens.snippet.h"
};

/* for debugging? */
const string token_names[] = {
#define X(ident, lit) LITSTR(lit)
#include "for_tokens.snippet.h"
};

const string token_name(enum token_type type) {
	switch(type) {
#define X(ident, val) \
		case TOKEN_ ## ident: \
			return LITSTR(val);
#include "for_tokens.snippet.h"
	};
}


bool find(struct parser* p, string needle) {
	const char* res = memmem(
		p->data + p->cur,
		p->len - p->cur,
		needle.base, needle.len);
	if(res == NULL) return false;
	p->cur = res - p->data;
	return true;
}

bool pass(struct parser* p, enum token_type tok) {
	const string what = token_name(tok);
	if(0 == memcmp(
		   p->data + p->cur,
		   p->len - p->cur,
		   what.base, what.len)) {
		p->cur += what.len;
		return true;
	}
	return false;
}



void raw_parse(struct parser* p) {
	if(pass(p, TOKEN_Q)

void root_parse(struct parser* p) {
	if(p->start_mode == STRIFY) {
		strify_parse(p);
	} else {
		raw_parse(p);
	}
}
	
