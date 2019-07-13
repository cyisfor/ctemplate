#define _GNU_SOURCE // memmem
/* use mmap for any files outside this function */

#include "c_generate.h"
#include "internal_output.h"
#include "processing_type.h"
#include "note.h"

#include "mystring.h"
#include <string.h> // memmem
#include <ctype.h> // isspace

#define output_string(str) fwrite(str.base, str.len, 1, out)
#define output_literal(lit) fwrite(lit, sizeof(lit)-1, 1, out)
#define output_char(c) fputc(c, out)
#define output_escaped(unesc) output_escaped(out, unesc)

enum paren_types {
	PAREN,
	SQUARE_BRACKET,
	CURLY_BRACKET,
	ANGLE_BRACKET,
	DOUBLEQUOTE,
	C_COMMENT,
	C_LINE_COMMENT
};

enum paren_direction {
	OPEN_PAREN,
	CLOSE_PAREN
};

#define MAX_PARENS 255

struct parser {
	string in;
	size_t cur;
	size_t start_string;
	size_t end_string;
	size_t start_code;
	FILE* out;
	enum processing_type type;
	unsigned char nparens;
	enum paren_types parens[MAX_PARENS];
	size_t close_paren_length;
};

bool find_and_pass(struct parser* p, string needle) {
	const char* s = memmem(p->in.base + p->cur, p->in.len - p->cur,
									needle.base, needle.len);
	if(s == NULL) return false;
	p->cur = (s - p->in.base) + needle.len;
	return true;
}

bool find_and_pass_char(struct parser* p, unsigned char needle) {
	const char* s = memchr(p->in.base + p->cur, needle, p->in.len - p->cur);
	if(s == NULL) return false;
	p->cur = s - p->in.base + 1;
	return true;
}

void commit_string(struct parser* p) {
	string s = {
		.base = p->in.base + p->start_string,
		.len = p->end_string - p->start_string
	};
	p->start_string = p->end_string+1; // XXX: +1?
	if(s.len == 0) return; // allows for files starting in ctemplate {}
	FILE* out = p->out;
	output_literal("output_literal(\"");
	output_escaped(s);
	output_literal("\");"); /* XXX: semicolon? do { ... } while(0)? */
}

static
void commit_rest(struct parser* p) {
	p->end_string = p->in.len;
	p->cur = p->in.len;
	commit_string(p);
}

static
void pass_space(struct parser* p) {
	while(p->cur < p->in.len && isspace(p->in.base[p->cur])) {
		++p->cur;
	}	
}

static
bool pass(struct parser* p, string needle) {
	if(p->in.len <= p->cur + needle.len) return false;
	if(0 == memcmp(p->in.base + p->cur,
				   needle.base,
				   needle.len)) {
		p->cur += needle.len;
		return true;
	}
	return false;
}

static
bool pass_char(struct parser* p, unsigned char c) {
	if(p->in.len == p->cur) return false;
	if(p->in.base[p->cur] == c) {
		++p->cur;
		return true;
	}
	return false;
}

static
void add_paren(struct parser* p, enum paren_types paren) {
	++p->nparens;
	assert(p->nparens < MAX_PARENS);
	p->parens[p->nparens-1] = paren;
	switch(paren) {
	case C_COMMENT:
		p->close_paren_length += 2;
		break;
	default:
		++p->close_paren_length;
		break;
	};
}

static
bool pass_open_paren(struct parser* p) {
	bool foundsome = false;
	for(;;) {
		if(pass_char(p, '(')) {
			add_paren(p, PAREN);
		} else if(pass_char(p, '[')) {
			add_paren(p, SQUARE_BRACKET);
		} else if(pass_char(p, '{')) {
			add_paren(p, CURLY_BRACKET);
		} else if(pass_char(p, '<')) {
			add_paren(p, ANGLE_BRACKET);
		} else if(pass_char(p, '"')) {
			add_paren(p, DOUBLEQUOTE);
		} else if(pass(p, LITSTR("/*"))) {
			add_paren(p, C_COMMENT);
		} else if(pass(p, LITSTR("//"))) {
			add_paren(p, C_LINE_COMMENT);
		} else {
			return foundsome;
		}
		foundsome = true;
	}
}

#define FUNCTION_NAME find_and_pass_first_paren
#define DOSTRING find_and_pass
#define DOCHAR find_and_pass_char
#include "paren_search.snippet.h"

#define FUNCTION_NAME pass_next_paren
#define DOSTRING pass
#define DOCHAR pass_char
#include "paren_search.snippet.h"

bool finish_paren(struct parser* p, enum paren_direction direction) {
	unsigned char i;
	size_t start = p->cur;
	for(i=1;i<p->nparens;++i) {
		if(!pass_next_paren(p, p->parens[i], direction)) {
			p->cur = start;
			return false;
		}
	}
	return true;
}

bool find_and_pass_parenthetical_block(struct parser* p) {
	if(p->nparens == 0) {
		ERROR("no parens exist to close?");
	}
	size_t oldcur = p->cur;
	bool open_found = find_and_pass_first_paren(p, p->parens[0], OPEN_PAREN);
	if(!open_found) {
		p->cur = oldcur;
		return false;
	}
	if(!finish_paren(p, OPEN_PAREN)) {
		p->cur = oldcur;
		return false;
	}
	p->start_code = p->cur;
	int level = 1;
	for(;;) {
		size_t derp = p->cur;
		bool open_found = find_and_pass_first_paren(p, p->parens[0], OPEN_PAREN);
		size_t open_pos = p->cur;
		bool close_found = find_and_pass_first_paren(p, p->parens[0], CLOSE_PAREN);
		size_t close_pos = p->cur;
		if(open_found && close_found) {
			if(open_pos < close_pos) {
				p->cur = open_pos;
				if(finish_paren(p, OPEN_PAREN)) {
					++level;
				}
				if(level != 0) {
					p->cur = close_pos;
					if(finish_paren(p, CLOSE_PAREN)) {
						assert(level > 0);
						--level;
						if(level == 0) break;
					}
				}
			} else {
				assert(level != 0);
				p->cur = close_pos;
				if(finish_paren(p, CLOSE_PAREN)) {
					assert(level > 0);
					--level;
					if(level == 0) break;
				}
				if(finish_paren(p, OPEN_PAREN)) {
					++level;
				}
			}
		} else if(open_found) {
			// !close_found
			ERROR("unbalanced parentheses, since no close found but level > 0");
		} else {
			if(finish_paren(p, CLOSE_PAREN)) {
				assert(level > 0);
				--level;
				if(level == 0) break;
			}
		}
	}
	p->nparens = 0;
	p->close_paren_length = 0;
	return true;
}

static void process_code(struct parser* p, string code) {
	FILE* out = p->out;
	commit_string(p);
	switch(p->type) {
	case processing_type_UNKNOWN:
		output_string(code);
		break;
	case processing_type_ESCAPED:
		output_char('"');
		output_escaped(code);
		output_char('"');
		break;
	default:
		switch(p->type) {
		case processing_type_OUTPUT_LITERAL:
			output_literal("output_literal(");
			break;
		case processing_type_OUTPUT_STRING:
			output_literal("output_string(");
			output_string(code);
			output_literal(")");
			break;
		case processing_type_OUTPUT_FORMAT:
			output_literal("output_format(");
			break;
		};
		output_string(code);
		output_literal(");"); // XXX: semicolon? do { } while(0) ?
	};
}

bool pass_statement(struct parser* p, string tag) {
	// return false only when no further statements can be found.
	if(!find_and_pass(p, tag)) {
		return false;
	}
	p->end_string = p->cur - tag.len;
	pass_space(p);
	if(pass_char(p, '(')) {
		size_t start_type = p->cur;
		if(!find_and_pass_char(p,')')) {
			WARN("EOF after opening ctemplate (type) paren without closing");
			p->end_string += tag.len;
			p->cur = p->end_string+1;
			return true;
		}
		string typestr = {
			.base = p->in.base + p->cur,
			.len = p->cur - 1 - start_type
		};
		if(typestr.len == 0) {
			WARN("empty type string?");
			p->end_string += tag.len;
			p->cur = p->end_string+1;
			return true;
		}
		enum processing_type type =
			lookup_processing_type(typestr.base, typestr.len);
		p->type = type;
		if(type == processing_type_UNKNOWN) {
			// probably unspecified type ctemplate (somecode += 42)
			add_paren(p, PAREN);
			++p->cur;
		}
	}
	pass_space(p);
	if(find_and_pass_parenthetical_block(p)) {
		// close_paren_length in characters NOT tokens
		string code = {
			.base = p->in.base + p->start_code,
			.len = p->cur - p->start_code - p->close_paren_length
		};
		process_code(p, code);
	} else {
		// XXX: this probably shouldn't even be a warning
		WARN("ctemplate token found without open paren");
		p->end_string = p->cur;
		p->cur = p->end_string+1;
		return true;
	}
}


void generate(FILE* out, string in, struct generate_options opt) {
	if(opt.tag.base == NULL) {
		opt.tag = LITSTR("ctemplate");
	}
	struct parser p = {
		.out = out,
		.in = in
	};
	while(pass_statement(&p, opt.tag));
	commit_rest(&p);
}
