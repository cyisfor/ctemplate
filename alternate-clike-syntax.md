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

int bar = 23; Q (/*" int foo = 42;"*/) would tokenize to

"int bar = 23; "
Q
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

So like Q((this is the parens (( and )))) would be valid, and "(( and ))" inside it would be skipped while searching for the close_paren token. 

The parser should be able to be set “strict” where `Q[(stuff])` fails since the opener is `[(` but there’s no closing `)]` but be by default permissive, where `Q[(stuff])` evaluates to itself.

The tag should be identifier-y, so stuff like `abcdQ(something)` would not result in quoting something. `abcd Q(something)` would work anyway, because trailing space after abcd is removed.

Track indentation by `{}` characters? A `{` can be followed by a newline and increasing the indent. A `}` decreases the indent, and can be preceded by a newline after doing so, and followed by a newline.

Indentation inside quoted text is tracked based on the outermost quote. So `abcd { efg { Q { hij { klm } } Q{nop} } qrs } tuv` could become:
```C
abcd {
	efg {
		"hij {\n\tklm\n}\n"
		"nop"
	}
	qrs
}
tuv
```

So the indent of the raw code is 2 at “hij” but the indent within the quoted text is 0.

`abcd { Q { efg { hij } Q { { klm Q({ KLM }) } noop Q { qrs } } } } tuv`
=>
```C
abcd {
	"efg {\n\thij\n}\n" {
		klm
		"{\n\t\tKLM\n\t}\n"
	}
	noop
	"qrs"
}
tuv
```


uhhhhhhhhhhhh

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
const char* code = output_literal("int foo = 42;\n  if(bar == 23) {\n    foo = 23;\n  }");
```
In general, trailing and leading space is removed. Space indenting the line after “Q” is removed from each line within the quote. All other unquoted space is preserved verbatim. 

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

quasi-quoting is... tricky. Firstly, string literals don’t save length information when being passed to functions, secondly, number of arguments does not pass down either. In general, just unquote/quote to put quasi-stuff inside. 

TODO: have `QQ{{this is a {{"quasi-quote"}} syntax that has the {{"first"}} level of nested parentheses act as an un-quote}}`
=>
`output_literal("this is a ") "quasi-quote" output_literal(" syntax that has the ") "first" output_literal(" level of nested parentheses act as an un-quote")`

As a method to keep C formatters from flipping out about unclosed parentheses, have some "parentheses" like
```
_ / _
OPEN / CLOSE
CLOSE / OPEN
START / END
END / START
CODE / TEMPLATE
```

and an optional `;` at the end of open OR close that gets skipped.

So...
```C
#include "output.h"
int main(int argc, char** argv) {
	FILE* out = stdout;
#define output_literal(lit) fwrite(lit,sizeof(lit)-1,1,out)
    Q {
#include <stdio.h>	
#define DERP "Q(fputs(argv[1],out))"
	}; 
	fputs("lolidk",stderr);
	int i;
	Q {
	int main(void) {
		/* this is an unrolled for loop: */
		Q {
			for(i=1;i&lt;=10;++i) {
				Q {
					printf("%d\n",19+Q(printf("%d",13+i)));
				}
			}
		}
		fputs(DERP, stdout);
		return 0;
	}
	}
	return 0;
}
```
=>
```C
#include "output.h"
int main(int argc, char** argv) {
	FILE* out = stdout;
#define output_literal(lit) fwrite(lit,sizeof(lit)-1,1,out)
	output_literal("#define DERP \""); 
	fputs(args[1],out);
	output_literal("\"\n");
	fputs("lolidk",stderr);
	int i;
	output_literal("int main(void) {\n\t/* this is an unrolled for loop: */\n");
	for(i=1;i&lt;=10;++i) {
		output_literal("\tprintf(\"%d \",19+");");
		printf("%d",13+i);
		output_literal(");\n");
	}
	output_literal("\tfputs(DERP, stdout);\n}\n");
}
```

Which would presumably output the program
`./thatthing myargument`
=>
```C
#include <stdio.h>
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

Soooo...

```
(tabs A)(code1)Q(space)(open quote)(space)
(tabs B)(nonspace-characters)Q(space)(open-quote2)(space)(code2)(space)(close-quote2)
(tabs B)(tabs C)(more nonspace characters)
(tabs A)(close quote)
```
=>
```
(tabs A)(code1)output_literal("(nonspace-characters)\n");(code2)
(tabs A)output_literal("(tabs C)(more nonspace characters)\n")
```

So parse line by line, but keep a stack of nested blocks, marking the ones that end a Q section specially, so that they can change the output method. Anything inside a Q block is cached up to the first inner Q block, or the close bracket, then output as an `output_literal(...)`. Anything else is copied verbatim, code to code. Anything inside an inner Q block is copied verbatim, then when the block closes, it starts a new `output_literal()` thingy.

And of course “tokens” (Q, open quote, close quote) can’t have embedded newlines. Should be able to write Q and then a newline though.

So raw mode, output literally until find Q. Then maybe quote mode.
maybe quote mode, save up until non-space or open-paren.
- raw mode
  - output literally
  - if Q, set Q found, start looking for non-space
- looking for non-space:
  - save space
  - newline counts as space
  - if open-paren, 
    - push raw mode
    - go to looking-for-first-tab mode
	- remember tabs of current line for outputting literally
  - if non-space:
    - cancel mode switch
	- output Q then space then non-space
	- unset Q found
	- go back to raw mode
- looking-for-first-tab mode, do common quote mode stuff, otherwise:
  - remember tabs of current line
	- go to quoted mode
- common quote mode stuff:
  - if open quote with no Q, increase nesting level but change no mode
  - nesting level is just to let open/close quotes match, instead of not allowing close quotes
  - if Q, set Q found, start looking for non-space
- quoted mode:
  - do common quote mode stuff
  

When beginning a Q block, keep parsing by line, find the tabs until the first non-tab character, and remove those from each line. Then output the result of that as an `output_literal()` thingy. 



No “types” of ctemplate processor, it just switches between template mode `Q("...")` and raw mode that just copies the code verbatim. 

It probably would be bad to start in raw mode, because raw mode isn’t supposed to be parsed at all, so we wouldn’t be able to search for the ctemplate tag in it. If we did that, we might as well use m4 which reveals what a nightmare it is
when you try to output verbatim, except for XXX, rather than outputting XXX, except sometimes verbatim.

TODO: switch between starting in raw or starting in quoted mode?
