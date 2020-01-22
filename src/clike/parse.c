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

};

struct parser {
	int outer_indent;
	int inner_indent;
	byte* data;
	int len;
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
	const char* res;
	int additional = 0;
	if(needle.len == 1) {
		res = memchr(p->data + p->cur,
					 *needle.base,
					 p->len - p->cur);
		if(res == NULL) return false;
	} else {
		res = memmem(
			p->data + p->cur,
			p->len - p->cur,
			needle.base, needle.len);
		if(res == NULL) return false;
		/* if it's a multi-character token, then there must be non-alphanumeric stuff on either side
		   that way Q { STARTLED FRIEND  } won't parse to Q { START LED FRI END } and
		   "LED FRI"
		   TODO: replace isalnum w/ custom function that also does _, -, ., etc...
		   XXX: UTF-8 characters count as non-alnum, so... should it be non-non-alnum?
		   Like space + some punctuation and everything else can't split?

		   In any case single character tokens are all brackets, so they don't make sense to worry
		   about what's around them.
		 */
		if(res > p->data && isalnum(res[-1])) return false;
		if(res < p->data + p->len) {
			if(isalnum(res[needle.len])) return false;
			if(res[needle.len] == ';' || res[needle.len] == ':') {
				/* OPEN; is the same as OPEN
				 that's to help auto-indenting editors not throw a hissy fit*/
				additional = 1;
			}
		}
	}
	p->cur = res - p->data + additional;
	return true;
}

void incr(struct parser* p, int len) {
	assert(p->len - p->cur >= len);
	p->cur += len;
}

bool find_and_pass(struct parser* p, enum token_type tok) {
	bool ret = find(p, tok);
	if(ret) {
		const string val = token_name(tok);
		incr(p, val.len);
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


bool find_and_pass_token_sequence(struct parser* p, enum token_type* tok, int num) {
	int i;
	int save = savepoint(p);
	for(i=0;i<num;++i) {
		/* XXX:
		   but 
		 */
		eat_space(p);
		if(!find_and_pass(p, tok[i])) {
			restore(p, save);
			return false;
		}
	}
	eat_space(p);
	return true;
}
	

bool pass(struct parser* p, enum token_type tok) {
	const string what = token_name(tok);
	if(0 == memcmp(
		   p->data + p->cur,
		   p->len - p->cur,
		   what.base, what.len)) {
		incr(p, what.len);
		return true;
	}
	return false;
}

bool pass_token_sequence(struct parser* p, enum token_type* tok, int num) {
	int save = savepoint(p);
	for(i=0;i<num;++i) {
		if(!pass(p, tok[i])) {
			restore(p, save);
			return false;
		}
	}
	return true;
}

/*
Note: Q {{({( a} b} c} d} )})}} should become " a } b} c} d} "
so ONLY "{{({(" can increase the depth, and we don't have to search for many possible
token sequences.

 */
struct paren_stack {
	enum token_type* open;
	enum token_type* close;
	int size;
	int depth;
};

void free_par(struct paren_stack par) {
	int i;
	for(i=0;i<par.levels;++i) {
		g_free(par.levels[i].tokens);
	};
	g_slice_free1(sizeof(*par.levels) * par.size, par.levels);
}

void check_after_Q(struct parser* p) {
	eat_space(p);
	/* XXX: must check after Q to switch to template mode
	 BUT also if in template mode, back to raw mode!
	*/
#define P(ident, open, close) if(pass(p, TOKEN_ ## ident)) { if(start_template(p, TOKEN_ ## ident)) return; }
#include "for_tokens.snippet.h"
#endif	

bool template_parse(struct parser* p, struct paren* par) {
	int save = savepoint(p);
	for(;;) {
		if(pass_token_sequence(p, par->open, par->size)) {
			++p->inner_indent;
			++par->depth;
		} else if(pass_token_sequence(p, par->close, par->size)) {
			if(par->depth == 0) return true;
			--p->inner_indent;
			--par->depth;
		} else {
			if(pass(p, TOKEN_Q)) {
				
			}
			// non-parenthetical stuff, but may find next parenthetical!
			int savederp = savepoint(p);
			bool a = find_and_pass_token_sequence(p, par->open, par->size);
			int savea = savepoint(p);
			restore(p, savederp);
			bool b = find_and_pass_token_sequence(p, par->close, par->size);
			int saveb = savepoint(p);
			if(!b) {
				/* unbalanced parentheses! */
				restore(p, save);
				return false;
			}
			if(a) {
				if(savea < saveb) {
					/* a 1... ( 2... ) situation. Unfortunately 1... may be ((( so (((()))) would give us "(((()"
					   between the first open and first close. So have to just start parsing ++--level
					   at the lowest one.
					   We know there's no ) in 1... so we can skip that at least.
					*/
					++p->inner_indent;
					++par->depth;
					restore(p, savea);
				} else {
					/* a 1... ) 2... ( situation. May end the parsing. Also we know 1... doesn't contain ( */
					/* we're already at saveb, no need to restore */
					if(par->depth == 0) {
						/* start parsing after the ), have to parse the ( twice... */
						return true;
					}
					--p->inner_indent;
					--par->depth;
				}
			} else {
				/* we're already past saveb */
				if(par->depth == 0) return true;
				--p->inner_indent;
				--par->depth;
			}
		}
	}
}

#define P(tok, open, close) if(pass(p, TOKEN_ ## tok)) { start_new_par(par, TOKEN_ ## tok); search_for_more_parens_
		forward(p, 1);
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
#define P(tok, open, close) if(pass(p, TOKEN_ ## tok)) { \
			++p.levels[0].ntokens;						\
			g_renew(p.levels[0].tokens, p.levels[0].ntokens); \
			p.levels[0].tokens[p.levels[0].ntokens-1] = TOKEN_ ## tok; \
			continue; \
		}
#include "for_parens.snippet.h"

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
	if(!find_and_pass(p, TOKEN_Q)) {
		finish();
		return;
	}
	check_after_Q(p);
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
