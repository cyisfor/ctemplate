A super simple, super stupid templating system for C. "Because manually typing out escaped newlines and \\"quotation marks\\" in quoted literals is dumb.\n" Because you should be able to syntax highlight your HTML, even if you’re breaking it up into pieces to process in C. Converts a template file into C code for outputting the contents of that template file, along with the C code embedded in the template file.

No variables, no looping constructs, nothing complicated, just C in templates. Syntax ripped off from XML processing instructions

`before <?C code ?> after`

=>
```C
output_literal("before ");
code;
output_literal(" after");
```

You need to define "output_literal" and stuff in code, then include this system’s generated source. See `output.h` `test.template.html` `example.c` and `Makefile` for an example of how to do this sort of thing.

```C
#define output_literal(lit) strreserve(buf,sizeof(lit)-1); \
	memcpy(buf.base+buf.off,lit,sizeof(lit)-1); buf.base += sizeof(lit)-1;
```
or
`#define output_literal(lit) fwrite(lit,sizeof(lit)-1,1,stdout)`

or w/ev

?C is case sensitive, because...

`<?c code ?>`

=>
`output_fmt(code)`

which is either snprintf, or fprintf, or your own output_fmt

```C
#define output_fmt(fmt,...) fprintf(stdout,fmt, ## __VA_ARG__)
<?c "these are some values %d %d %d", 1, 2, 3 ?>
```

This is stupid, but:
```C
#define output_fmt(a,b,c) if(a) { puts(b); } else { puts(c); }
<?c sometest(), "yea", "nay" ?>
```
Better to do this:
`<?cl lit ?>`

=> `output_literal(lit)`

that could be good for like...
`<?cl sometest() ? "yea" : "nay" ?>`

To embed a `?>` use `\?>`, and string literal boundaries, parentheses, brackets etc are ignored.

`<?cl "This will fail ?>" ?>`

=>

`output_literal("This will fail);
output_literal("\" ?>");
`

`<?cl "This will not fail \?>" ?>`

=> `output_literal("This will not fail ?>");`

`<?cl "This also will not \\\?> fail \?>" ?>`

=> `output_literal("This also will \\?> not fail ?>");`

=> (outputs) `This also will \?> not fail ?>`

`\<?` will also work to avoid going into code mode.

`This is the enter code delimiter: \<?. <?cl cool_huh ? "cool!" : "not cool." ?>`

=>

```
output_literal("This is the enter code delimiter: <?. ");
output_literal(cool_huh ? "cool!" : "not cool.");
```


`<?cs buf, len ?>`

=> `output_buf(buf, len);`

`<?csl str ?>`

=> `{ const char* s = str; output_buf(s, strlen(s)); }`

`<?cSS S.s = "hi \0there"; S.l = 9; ?>`

=> `{ string S; S.s = "hi \0there"; S.l = 9; output_buf(S.s,S.l); }`

`<?cS thing.s = "defined elsewher"; thing ?>`

=> `{  string S = ({ thing.s = "defined elsewher"; thing; }); output_buf(S.s,S.l); }`

`<?cS(name) name.s = "hi \0there"; name.l = 9; ?>`

=> `{ string name; name.s = "hi \0there"; name.l = 9; output_buf(name.s,name.l); }`

note this parser is very stupid. Basically expects the inside to be syntax that "works". So

`<?cS(thing) "oops" ?>`

=> `{ string thing; "oops" output_buf(thing.s, thing.l); }`

Note also that `<?cS` assumes a “string” struct is defined with members ‘s’ and ‘l’ for string and length. Probably should change it to `data` and `iov_base_process_size_requisition` or whatever “real” string structs use as members, like uv_buf_t or struct iovec.

if your template starts with `<?C` you can define output_etc at the top, I guess.

```C
<?C
#include <stdio.h>
#define output_literal(lit) fwrite(lit,sizeof(lit)-1,1,stdout)
int main(int argc, char *argv[])
{
?>
This will just output the text here, as a self contained program.
<?C	
	return 0;
}
```

Usually, whitespace before the first `<?C` will be consumed, and not output_literal'd but there might be an option to disable that I guess. If it's disabled, then whitespace before will be like...

```C
 <?C
#define output_literal(lit) puts("oops");
?>
...
```

=>

```C
output_literal(" ");
#define output_literal(lit) puts("oops");
output_literal(...);
...
```

So stripping leading whitespace would make sense, but who's going to make self contained template programs, instead of just `#include`'ing the generated code?

Note: to do looping constructs...

```C
Prefix
<?C
{ int i;
   for(i=0;i<=23;++i) {
?>
The value of i is <?c "%d", i+19 ?>.
<?C
} }
?>
```

In general though, avoid a lot of C syntax in templates. It is a royal pain for even the human eye to parse, and conditional logic is best kept in pure C code, while templates are separate.

TODO:

Optionally... I guess you could create a bunch of defines?
```C
The beginning
<?>
The middle %d
<?>
The end.
```
generated with the name “DERP” =>

```C
#define DERP0 "The beginning\n"
#define DERP1 "\nThe middle %d\n"
#define DERP2 "\nThe end.\n"
```

Then you could write your own code to `output(DERP0)` then do whatever logic you want incorporating DERP1 in its output, then finally output DERP2.
