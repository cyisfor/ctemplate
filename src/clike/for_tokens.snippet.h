#ifndef COMMA
#define COMMA ,
#endif
X(Q, "Q") COMMA
X(NEWLINE, "\n") COMMA
#define DEFPAREN(tok, open, close) X(TOKEN_ ## tok, open) COMMA X(TOKEN_ ## tok ## CLOSE, close)
#include "parens.snippet.h"
#undef COMMA
