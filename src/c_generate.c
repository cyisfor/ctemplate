/* use mmap for any files outside this function */

#define output_string(str) fwrite(str.base, str.len, 1, out)
#define output_literal(lit) fwrite(lit, sizeof(lit)-1, 1, out)
#define output_char(c) fputc(c, out)
#define output_escaped(unesc) output_escaped(out, unesc)

struct generate_options {
	string tag;
};

struct parser {
	string in;
	size_t cur;
	size_t start_string;
	size_t end_string;
	FILE* out;
};

bool find(const struct parser* p, string needle) {
	const char* s = memmem(p->in.base + p->cur, p->in.len - p->cur,
						   needle.base, needle.len);
	if(s == NULL) return false;
	p->cur = s - p->in.base;
	return true;
}

void commit_string(struct parser* p) {
	string s = {
		.base = p->in.base + p->start_string,
		.len = p->end_string - p->start_string
	};
	if(s.len == 0) return; // allows for files starting in ctemplate {}
	p->start_string = p->end_string+1; // XXX: +1?
	output_literal(p->out, "output_literal(\"");
	output_escaped(p->out, s);
	output_literal(p->out, "\");"); /* XXX: semicolon? do { ... } while(0)? */
}

static
void commit_rest(struct parser* p) {
	p->end_string = p->in.len;
	p->cur = p->in.len;
	commit_string(p);
}

static
void eat_space(struct parser* p) {
	while(isspace(p->in.base[p->cur])) {
		++p->cur;
	}	
}

bool find_tag(struct parser* p, string tag) {
	if(!pass(p, tag)) {
		commit_rest(p);
		return false;
	}
	p->end_string = p->cur - tag.len;
	pass_space(p);
	if(pass_char(p, '(')) {
		if(!pass_char(p,')')) {
			warn("EOF after opening ctemplate (type) paren without closing");
			commit_rest(p);
			return false;
		}
		string typestr = {
			.base = p->in.base + p->cur,
			.len = s - p->in.base + p->cur
		};
		enum processing_type type =
			lookup_processing_type(typestr.base, typestr.len);
		p->type = type;
		if(type == processing_type_UNKNOWN) {
			// probably unspecified type ctemplate (somecode += 42)
			add_open_paren(p, OPEN_PAREN);
			++p->cur;
		}
	}
	pass_space(p);
	if(pass_open_paren(p)) {
		// up to the end of the open paren
		size_t start_code = p->cur;
		if(pass_close_paren(p)) {
			// close_paren_length in characters NOT tokens
			string code = {
				.base = p->in.base + start_code,
				.len = p->cur - start_code - p->close_paren_length
			};
			process_code(p, code);
			return true;
		} else {
			warn("ctemplate token found without close paren!");
			// NOTE: pass_* MUST NOT advance if not found
			assert(p->cur == start_code);
			return false;
		}
	} else {
		// XXX: this probably shouldn't even be a warning
		warn("ctemplate token found without open paren");
		return false;
	}
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

		
		
		
		
			
	
			


void generate(FILE* out, string in, struct generate_options opt) {
	if(opt.tag.base == NULL) {
		opt.tag = LITSTR("ctemplate");
	}
	size_t start_string = 0;
	size_t end_string = 0;
	enum mode {
		FIND_TAG
	};

		
		
