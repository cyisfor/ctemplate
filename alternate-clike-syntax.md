So... generating C code is useful, and writing C code to generate C code is useful, and CPP sucks (you can’t #include inside macro arguments because “reasons” etc), but most text editors (cough)emacs(cough) throw a hissy fit when you mix any sort of non-C-like syntax in with C. 

Basically what we need is lisp’s quote but with a more C friendly syntax.

So... how about this syntax:

string quote eatspace open_paren code eatspace close_paren string

where quote is some "thisisctemplate_notafunction" identifier-like text, open_paren and close_paren are any of MANY opening/closing delimiters, such as
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

So first you look for the quote, and output non-matching characters as code. Then you skip any space, then check for a possible open_paren sequence of tokens. If found, you search for the matching close_paren sequence of tokens, while accounting for nested parentheticals.

If the close_paren IS found, then commit the current code, then process everything between open_paren and close_paren as a double quoted escaped string around output_literal. Start searching for the quote again after close_paren.

If the close_paren is not found, either error out, or add everything from the start_template token to the end of the open_paren as code (including space), then restart searching for a quote token from there.

"space" includes space, tab, newline, carriage return, so you could do like

```C
#define output_literal(lit) lit
  const char* code = Q {
    int foo = 42;
	if(bar == 23) {
	  foo = 23;
	}
  };
};
```

and that’d become:
```C
const char* code = "int foo = 42;\n  if(bar == 23) {\n    foo = 23;\n  }";
```
In general, trailing and leading space is removed. Space indenting the line with “Q” is removed from each line within the quote. All unquoted space is preserved verbatim. 

Not sure if " and " are useful open_paren and close_paren tokens...
`Q"why am I even using ctemplate"`
=>
`"why am I even using ctemplate"`

Of course `\` has to escape an unquote, so 
`Q"this is a double quote \" character"`
=>
`"this is a double quote \" character"`
not
`"this is a double\\" character"`

Even a complex close-paren like `})]>` could be escaped with `\})]>` but at that point just use a different complex close-paren, right? even `}}` is different from `}`. It would work though, even if only `}` was escaped because `)]>` won’t be the close quote.

quasi-quoting is... tricky. Firstly, string literals don’t save length information when being passed to functions, secondly, number of arguments does not pass down either. But something should work like this:

```C
char buf[20];
QQ{{this is a }}int num = 42; (string){buf, snprintf(buf, 20, "%d", num)}{{ test}}
```
=>
```C
char buf[20];
quasi_quote(3, LITSTR("this is a "),
  {
    int num = 42; (string){buf, snprintf(buf, 20, "%d", num)};
  },
  LITSTR(" test"))
```

definition of LITSTR can be found in mystring.h, and the string structure too. Definition of quasi_quote would be programmer dependent, but in general I guess it should output the arguments? like
```C
#include <stdarg.h>
#include <stdio.h>
void quasi_quote(size_t narg, ...) {
  va_list args;
  va_start(args, narg);
  size_t i;
  for(i=0;i<narg;++i) {
    string s = va_arg(args, string);
	fwrite(s.base, s.len, 1, stdout);
  }
}
```

It could also accumulate them and return a `string` so you could do uh...
```C
string result = QQ{...};
```

using static/#undef/#define for quasi_quote ought to help if that needs to vary.

in particular, `#define quasi_quote(N, ...) quasi_quote_function(out, N, ##__VA_ARGS__)` is useful, because `FILE* out = whatevercontext->output` can be an easy local variable.

ctemplate really doesn’t care, it just puts quasi_quote in, and LITSTR and hopes you defined them good. It does have to parse the quasi-quote statement twice, once to count the arguments, but it’s either that, or have an unbounded cache of integer offsets for the arguments of a quasi_quote statement.

XXX: that (string){etc} stuff is messy at the end. Can make neater?
............................

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
