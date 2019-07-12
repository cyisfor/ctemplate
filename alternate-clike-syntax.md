So... generating C code is useful, and writing C code to generate C code is useful, and CPP sucks (you can’t #include inside macro arguments because “reasons” etc), but most text editors (cough)emacs(cough) throw a hissy fit when you mix any sort of non-C-like syntax in with C. So... how about this syntax:

string tag space optional("(" type ")") space open_paren code close_paren string

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
string = "int bar = 23;"
start_ctemplate_thing
open_paren = (, /*, "
code = " int foo = 42;"
close_paren = ", */, )

so first you look for the start_template token, and queue up the non-matching characters as a string. Then you skip any space, then check for a possible open_paren sequence of tokens. If found, you search for the matching close_paren sequence of tokens, while accounting for nested parentheticals.

So like CTEMPDERP((this is the parens (( and )))) would be valid, and "(( and ))" inside it would be skipped while searching for the close_paren token.

If the close_paren IS found, then check between open_paren and close_paren to see if it’s a type qualifier. If so, prepare that type of code processor, then start searching for a new open_paren as above, but skip to the next instead of looking for another type qualifier.

If the close_paren is found (maybe the second time) then commit the current string, then process everything between open_paren and close_paren as code. Start searching for start_template again after close_paren.

If the close_paren is not found, issue a warning, add everything from the start_template token to the end of the open_paren as a string (including space), then restart searching for a start_template token from there.

"space" includes space, tab, newline, carriage return, so you could do like

```C
const char* code = 
  ctemplate(s) 
  {
    int foo = 42;
	if(bar == 23) {
	  foo = 23;
	}
  };
```

and that’d become:
```C
const char* code = "\n    int foo = 42;\n    if(bar == 23) {\n      foo = 23;\n    }\n";
```

In general space between “ctemplate stuff” is skipped, while space surrounding those tokens is preserved and output and/or processed as code.

Not sure if " and " are useful open_paren and close_paren tokens...
`ctemplate(s)"why am I even using ctemplate"`
=>
`"why am I even using ctemplate"

Various types of ctemplate:
no type = just output raw code
s: turn code into a valid double quoted C string literal
ol: output_literal(the raw code)
of: output_format(the raw code)
os: output_string(the raw code)

the meaning of output_literal, output_format, and output_string are up to the programmer, but it’s strongly recommended to have the first one assume its code evaluates to a string literal, the second one to take a string literal as a first format argument, ala printf, and the third to assume its code evaluates to a string object. These can all be used inside the code itself of course, so

`ctemplate(ol)(sometest() ? "foo" : "bar")`
would be exactly equivalent to
`ctemplate(output_literal(sometest() ? "foo" : "bar"))`
or even
```C
ctemplate 
  {output_literal(sometest() ? "foo" : "bar")}
```

That seems... really dumb, but mostly it’s for ctemplate(s), and the other types I might as well add as conveniences. Anyone who doesn’t use `of` or `os` doesn’t have to define output_format or output_string (or have any notion of a string object).

Currently the () around a ctemplate type must be `(` and `)` since I can’t think of any valid reason to complicate things by having code like
```C
ctemplate
{{
// hi there
s{{
}}
}}

[whyyyyy]
```
=>
`"whyyyyy"`

So only "()" around the type qualifier, no nested parentheses or whitespace inside them. Just the literal characters for each type.
