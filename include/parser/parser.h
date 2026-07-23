#ifndef parser_h
#define parser_h

#include "lexer/lexer.h"
#include "lib/ast.h"
#include <stdbool.h>

typedef struct {
  char *srcName;

  Lexer *lexer;

  bool hasBuffer;
  Token tBuffer;
} Parser;

Parser ParserCreate(Lexer *lexer);

Ast *Parse(Parser *parser, Diagnostic *d);

#endif
