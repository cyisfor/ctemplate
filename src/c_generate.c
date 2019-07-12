/* use mmap for any files outside this function */

struct generate_options {
	string tag;
};

const char* find(string haystack, size_t off, string needle) {
	return memmem(haystack.base + off, haystack.len - off,
				  needle.base, needle.len);
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
	void commit_string(void) {
		string s = {
			.base = in.base + start_string,
			.len = end_string - start_string
		};
		if(s.len == 0) return; // allows for files starting in ctemplate {}
		start_string = end_string+1; // XXX: +1?
		PUTLIT("output_literal(\"");
		put_escaped(s);
		PUTLIT("\");"); 		/* XXX: semicolon? do { ... } while(0)? */
	}
			
	for(;;) {
		switch(mode) {
		case FIND_TAG: {
			const char* s = find(in, end_string, opt.tag);
			if(s == NULL) {
				end_string = in.len;
				commit_string();
			}
			
		
		
