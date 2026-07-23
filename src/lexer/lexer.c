#include "lexer/lexer.h"
#include "lib/token.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char nextChar(Lexer *lexer) {
  if (lexer->c == '\0') {
    int c = fgetc(lexer->srcFile);

    if (c == '\n') {
      lexer->head.row += 1;
      lexer->head.col = 1;
    } else {
      lexer->head.col += 1;
    }

    if (c == EOF) {
      return '\0';
    }

    return (char)c;
  }

  char c = lexer->c;

  if (c == '\n') {
    lexer->head.row += 1;
    lexer->head.col = 1;
  } else {
    lexer->head.col += 1;
  }

  lexer->c = '\0';
  return c;
}

Status pushChar(Lexer *lexer, char c, Pt old_head) {
  if (lexer->c != '\0') {
    return ERR;
  }
  lexer->c = c;
  lexer->head = old_head;
  return OK;
}

Lexer LexerCreate(char *srcName, FILE *srcFile) {
  return (Lexer){.srcName = srcName,
                 .srcFile = srcFile,
                 .c = '\0',
                 .head = (Pt){.row = 1, .col = 1}};
}

Status scanInt(Lexer *lexer, Token *t, Diagnostic *d, Pt start) {
  int64_t old_n = 0, n = 0;

  while (true) {
    old_n = n;

    Pt end = lexer->head;
    char c = nextChar(lexer);
    if ('0' <= c && c <= '9') {
      n = 10 * n + (c - '0');
    } else {
      pushChar(lexer, c, end);
      *t = (Token){.type = TOKEN_INT,
                   .val.i = n,
                   .loc = (Loc){.start = start, .end = end}};
      return OK;
    }

    if (n / 10 != old_n) {
      *d = (Diagnostic){.loc = (Loc){.start = start, .end = lexer->head},
                        .src = lexer->srcName,
                        .msg = "integer overflow"};
      return ERR;
    }
  }

  *d = (Diagnostic){.loc = (Loc){.start = start, .end = lexer->head},
                    .src = lexer->srcName,
                    .msg = "unreachable"};
  return ERR;
}

Status scanSym(Lexer *lexer, Token *t, Diagnostic *d, Pt start) {
  size_t i = 0;
  const size_t max = 64;
  char sym[max];

  while (true) {
    Pt end = lexer->head;
    char c = nextChar(lexer);

    if (('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z') ||
        ('0' <= c && c <= '9') || c == '_' || c == '\'') {
      sym[i] = c;
      i += 1;
    } else {
      pushChar(lexer, c, end);

      sym[i] = '\0';
      i += 1;
      char *symOut = malloc(i * sizeof(char));
      strncpy(symOut, sym, i);

      *t = (Token){.type = TOKEN_SYM,
                   .val.s = symOut,
                   .loc = (Loc){.start = start, .end = end}};
      return OK;
    }

    if (i >= max) {
      *d = (Diagnostic){.loc = (Loc){.start = start, .end = lexer->head},
                        .src = lexer->srcName,
                        .msg = "max symbol length exceeded (63)"};
      return ERR;
    }
  }

  *d = (Diagnostic){.loc = (Loc){.start = start, .end = lexer->head},
                    .src = lexer->srcName,
                    .msg = "unreachable"};
  return ERR;
}

Status NextToken(Lexer *lexer, Token *t, Diagnostic *d) {
  Pt start = lexer->head;
  char c = nextChar(lexer);
  Pt end = lexer->head;
  Loc loc = {.start = start, .end = end};

  switch (c) {
  case '\0':
    *t = (Token){.type = TOKEN_EOF, .val = {0}, .loc = loc};
    return OK;
  case '\\':
    *t = (Token){.type = TOKEN_LAMBDA, .val = {0}, .loc = loc};
    return OK;
  case '.':
    *t = (Token){.type = TOKEN_DOT, .val = {0}, .loc = loc};
    return OK;
  case '(':
    *t = (Token){.type = TOKEN_LPAREN, .val = {0}, .loc = loc};
    return OK;
  case ')':
    *t = (Token){.type = TOKEN_RPAREN, .val = {0}, .loc = loc};
    return OK;
  case '~':
    *t = (Token){.type = TOKEN_UNIT, .val = {0}, .loc = loc};
    return OK;
  case ' ':
  case '\t':
  case '\n':
    return NextToken(lexer, t, d);
  default:
    if ('0' <= c && c <= '9') {
      pushChar(lexer, c, start);
      return scanInt(lexer, t, d, start);
    }
    if (('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z') || c == '_') {
      pushChar(lexer, c, start);
      return scanSym(lexer, t, d, start);
    }

    *t = (Token){.type = TOKEN_OP, .val.c = c, .loc = loc};
    return OK;
  }
}
