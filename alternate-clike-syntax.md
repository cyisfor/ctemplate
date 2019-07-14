So... generating C code is useful, and writing C code to generate C code is useful, and CPP sucks (you can’t #include inside macro arguments because “reasons” etc), but most text editors (cough)emacs(cough) throw a hissy fit when you mix any sort of non-C-like syntax in with C. 

Basically what we need is lisp’s quasiquote/unquote but with a more C friendly syntax.

So... how about this syntax:

string tag eatspace open_paren code eatspace close_paren string

where tag is some "thisisctemplate_notafunction" identifier-like text, open_paren and close_paren are any of MANY opening/closing delimiters, such as
( and )
(( and ))
{ and }
" and "
` and '
' and '
[({< and >})]
/* and */
// and newline?

and so on. So for every token in open_paren, there's a lookup table mapping to the corresponding token in close_paren, and only a sequence of those mapped tokens counts as the close_paren for a given open_paren.

int bar = 23; ctemplate (/*" int foo = 42;"*/) would tokenize to

"int bar = 23; "
ctemplate
space = " "
(
/*
\"
" int foo = 42;"
\"
*/
)

which would semantically become:
code = "int bar = 23;"
tag
open_paren = (, /*, "
string = " int foo = 42;"
close_paren = ", */, )

So like ctemplate((this is the parens (( and )))) would be valid, and "(( and ))" inside it would be skipped while searching for the close_paren token. 

The parser should be able to be set “strict” where `ctemplate[(stuff])` fails since the opener is `[(` but there’s no closing `)]` but be by default permissive, where `ctemplate[(stuff])` evaluates to itself.

So first you look for the tag, and queue up the non-matching characters as a string. Then you skip any space, then check for a possible open_paren sequence of tokens. If found, you search for the matching close_paren sequence of tokens, while accounting for nested parentheticals.

If the close_paren IS found, then commit the current code, then process everything between open_paren and close_paren as a double quoted escaped string around output_literal. Start searching for the tag again after close_paren.

If the close_paren is not found, either error out, or add everything from the start_template token to the end of the open_paren as a string (including space), then restart searching for a start_template token from there.

"space" includes space, tab, newline, carriage return, so you could do like

```C
#define output_literal(lit) lit
  const char* code = CT {
    int foo = 42;
	if(bar == 23) {
	  foo = 23;
	}
  };
};
```

and that’d become:
```C
const char* code = "\n    int foo = 42;\n    if(bar == 23) {\n      foo = 23;\n    }\n";
```

Maybe an option to eat that trailing space? In general space between “ctemplate stuff” is skipped, while space surrounding it is preserved and output and/or stringized.

Not sure if " and " are useful open_paren and close_paren tokens...
`ctemplate"why am I even using ctemplate"`
=>
`output_literal("why am I even using ctemplate")`

The meaning of output_literal is up to the programmer, but it’s strongly recommended to have it be something like:
```C
#define output_literal(lit) fwrite(lit,sizeof(lit)-1,1,out)
```
where `out` is a local variable, possibly set from a passed context, as in:

```C
CT {
int output_one_record(struct context* ctx, struct record* rec) {
  FILE* out = ctx->output_file;
  }CT("""
  the name is CT{
  output_string(rec->name)} and the value is CT(output_string(rec->value))
  """)CT{
  return 42;
}
```
=>
```C
int output_one_record(struct context* ctx, struct record* rec) {
  FILE* out = ctx->output_file;
  output_literal("\n  the name is ");
  output_string(rec->name);
output_literal(" and the value is ");output_string(rec->value);
output_literal("\n  ");
  return 42;
}
```

You could also define output_literal like...
```C
#define output_literal(lit) lit
CT {
	const char* code = }
this is a literal string wheee
CT{
}
```
=>
```C
const char* code = "\nthis is a literal string wheee\n";
```

As a method to keep C formatters from flipping out about unclosed parentheses, have some "parentheses" like
```
_ / _
OPEN / CLOSE
CLOSE / OPEN
START / END
END / START
CODE / TEMPLATE
```

and an optional `;` at the end that gets skipped.

So...
```C
#include "output.h"
int main(int argc, char** argv) {
	FILE* out = stdout;
	CT_TEMPLATE;
#define DERP "CT(fputs(argv[1],out))"
	CODE_;
	fputs("lolidk",stderr);
	int i;
	CT_TEMPLATE;
int main(void) {
	/* this is an unrolled for loop: */
CODE_;
	for(i=0;i<10;++i) {
		CT {
		printf("%d\n",19+CT(printf("%d",13+i)));
		}
	}
	CT {
	fputs(DERP, stdout);
	}
	CT_TEMPLATE;
}
CODE_;
}

```
=>
```C
#include "output.h"
int main(int argc, char** argv) {
	FILE* out = stdout;
	output_literal("\n#define DERP \"");
	fputs(argv[1],out);
	output_literal("\"\n\t");
	fputs("lolidk",stderr);
	int i;
	mehhhhhhhhhhh do we delete the trailing whitespace or not?
	output_literal("\nint main(void) {\n\t/* this is an unrolled for loop: */\n");
		for(i=0;i<10;++i) {
			output_literal("printf(\"%d\\n\",19+");
printf("%d",14+i);
output_literal(");\n\t\t\t");
		};
output_literal("\n\t\t\tfputs(DERP, stdout);\n\t\t}");
}
```

Which would presumably output the program
`./thatthing myargument`
=>
```C
#define DERP "myargument"

	int main(void) {
		/* this is an unrolled for loop: */
		printf("%d ",19+14);
		printf("%d ",19+15);
		printf("%d ",19+16);
		printf("%d ",19+17);
		printf("%d ",19+18);
		printf("%d ",19+19);
		printf("%d ",19+20);
		printf("%d ",19+21);				
		printf("%d ",19+22);
		printf("%d ",19+23);
		fputs(DERP, stdout);
	}
```

...which would presumably output:
```
33 34 35 36 37 38 39 40 41 42 myargument
```

No “types” of ctemplate processor, it just switches between template mode `output_literal("...")` and raw mode that just copies the code verbatim. 

It probably would be bad to start in raw mode, because raw mode isn’t supposed to be parsed at all, so we wouldn’t be able to search for the ctemplate tag in it. If we did that, we might as well use m4 which reveals what a nightmare it is
when you try to output verbatim, except for XXX, rather than outputting XXX, except sometimes verbatim.
