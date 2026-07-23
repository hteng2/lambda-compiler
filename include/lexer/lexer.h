#ifndef lexer_h
#define lexer_h

#include "lib/diagnostic.h"
#include "lib/status.h"
#include "lib/token.h"
#include <stdint.h>
#include <stdio.h>

typedef struct {
  char *srcName;
  FILE *srcFile;
  char c;
  Pt head;
} Lexer;

Lexer LexerCreate(char *srcName, FILE *srcFile);

Status NextToken(Lexer *lexer, Token *t, Diagnostic *d);

#endif
