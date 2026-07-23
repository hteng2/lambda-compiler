#ifndef token_h
#define token_h

#include "loc.h"

typedef enum {
  TOKEN_EOF,
  TOKEN_LAMBDA,
  TOKEN_DOT,
  TOKEN_UNIT,
  TOKEN_AT,
  TOKEN_LPAREN,
  TOKEN_RPAREN,
  TOKEN_INT,
  TOKEN_OP,
  TOKEN_SYM,
} TokenType;

typedef struct {
  TokenType type;
  union {
    int64_t i;
    char c;
    char *s;
  } val;
  Loc loc;
} Token;

#endif
