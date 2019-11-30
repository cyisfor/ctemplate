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

bool find(struct parser* p, enum token_type tok) {
	const string needle = token_name(tok);
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

int savepoint(struct parser* p) {
	return p->cur;
}

void restore(struct parser* p, int savepoint) {
	p->cur = savepoint;
}

struct paren {
	enum token_type *tokens;
	int ntokens;
};

struct paren_stack {
	struct paren* levels;
	int size;
};

void free_par(struct paren_stack par) {
	int i;
	for(i=0;i<par.levels;++i) {
		g_free(par.levels[i].tokens);
	};
	g_slice_free1(sizeof(*par.levels) * par.size, par.levels);
}

void template_parse(struct parser* p, struct paren* par) {
	int save = savepoint(p);
	if(

bool start_template(struct parser* p, enum token_type tok) {
	if(done(p)) return false;
	/* need to find all the tokens in open paren thingy */
	struct paren_stack par = {
		.levels = g_slice_new(struct paren);
		.size = 0;
	};
	par.levels[0].tokens = g_new(enum token_type, 1);
	par.levels[0].ntokens = 1;

	par.levels[0].tokens[0] = tok;

	for(;;) {
		eat_space(p);
		if(done(p)) {
			free_par(par);
			return false;
		}
#define ONLY_PARENS
#define X(tok, val) if(pass(p, tok)) { \
			++p.levels[0].ntokens;						\
			g_renew(p.levels[0].tokens, p.levels[0].ntokens); \
			p.levels[0].tokens[p.levels[0].ntokens-1] = tok; \
			continue; \
		}
#include "for_tokens.snippet.h"
		
		bool ret = template_parse(p, &par);
		free_par(par);

		return ret;
	}

void raw_parse(struct parser* p) {
	void finish(void) {
		output_literal(
			p->out,
			p->data + p->cur,
			p->len - p->cur);
		p->cur = p->len;
	}
	
	int save = savepoint(p);
	if(!find(p, TOKEN_Q)) {
		finish();
		return;
	}
	eat_space(p);
#define ONLY_PARENS
#define X(ident, val) if(pass(p, TOKEN_ ## ident)) { if(start_template(p, TOKEN_ ## ident)) return; }
#include "for_tokens.snippet.h"
#endif
	restore(p, save);
	finish();
}

void root_parse(struct parser* p) {
	if(p->start_mode == TEMPLATE) {
		template_parse(p);
	} else {
		raw_parse(p);
	}
}
	
