#include "lib/ast.h"
#include "lib/token.h"
#include "parser/parser.h"
#include <stdint.h>
#include <stdlib.h>

#define APPLY_LEFT_BP 5
#define APPLY_RIGHT_BP 6
#define PREFIX_BP 7

typedef struct {
  uint8_t left, right;
} Bp;

BiOpType toBot(char c) {
  switch (c) {
  case '+':
    return BIOP_ADD;
  case '-':
    return BIOP_SUB;
  case '*':
    return BIOP_MUL;
  case '/':
    return BIOP_DIV;
  case '%':
    return BIOP_MOD;
  case '=':
    return BIOP_EQ;
  case '>':
    return BIOP_GT;
  case '<':
    return BIOP_LT;
  case '&':
    return BIOP_AND;
  case '|':
    return BIOP_OR;
  case '^':
    return BIOP_XOR;
  default:
    return BIOP_NONE;
  }
}

Bp infixBp(char c) {
  switch (c) {
  case '|':
    return (Bp){.left = 1, .right = 2};

  case '&':
  case '^':
    return (Bp){.left = 3, .right = 4};

  case '=':
  case '>':
  case '<':
    return (Bp){.left = 5, .right = 6};

  case '+':
  case '-':
    return (Bp){.left = 7, .right = 8};

  case '*':
  case '/':
  case '%':
    return (Bp){.left = 9, .right = 10};

  default:
    return (Bp){0};
  }
}

UnOpType toUot(char c) {
  switch (c) {
  case '-':
    return UNOP_NEG;
  case '!':
    return UNOP_NOT;
  default:
    return UNOP_NONE;
  }
}

Status getToken(Parser *parser, Token *t, Diagnostic *d) {
  if (parser->hasBuffer) {
    *t = parser->tBuffer;
    parser->hasBuffer = false;
    return OK;
  }

  return NextToken(parser->lexer, t, d);
}

Status pushToken(Parser *parser, Token t) {
  if (parser->hasBuffer) {
    return ERR;
  }
  parser->tBuffer = t;
  parser->hasBuffer = true;
  return OK;
}

Parser ParserCreate(Lexer *lexer) {
  return (Parser){
      .srcName = lexer->srcName,
      .lexer = lexer,
      .hasBuffer = false,
      .tBuffer = {0},
  };
}

Ast *parseLambda(Parser *parser, Diagnostic *d, Pt start);
Ast *parsePrefix(Parser *parser, char c, Loc loc, Diagnostic *d);
Ast *parseLeft(Parser *parser, uint8_t minBp, Diagnostic *d);
Ast *parseAdvance(Parser *parser, uint8_t minBp, Ast *left, Diagnostic *d);

Ast *parseLambda(Parser *parser, Diagnostic *d, Pt start) {
  Status s;

  Token param;
  s = getToken(parser, &param, d);
  if (s == ERR)
    return NULL;
  if (param.type != TOKEN_SYM) {
    *d = (Diagnostic){
        .loc = param.loc, .msg = "expected symbol", .src = parser->srcName};
    return NULL;
  }

  Token next;
  char *selfRef = NULL;
  Token dot;
  s = getToken(parser, &next, d);
  if (s == ERR)
    return NULL;
  if (next.type == TOKEN_AT) {
    Token name;
    s = getToken(parser, &name, d);
    if (s == ERR)
      return NULL;
    if (name.type != TOKEN_SYM) {
      *d = (Diagnostic){
          .loc = param.loc, .msg = "expected symbol", .src = parser->srcName};
      return NULL;
    }

    selfRef = name.val.s;
    s = getToken(parser, &dot, d);
  } else {
    dot = next;
  }

  if (s == ERR)
    return NULL;
  if (dot.type != TOKEN_DOT) {
    *d = (Diagnostic){
        .loc = dot.loc, .msg = "expected dot operator", .src = parser->srcName};
    return NULL;
  }

  Ast *expr = parseLeft(parser, 0, d);
  if (expr == NULL)
    return NULL;

  Ast *ast = malloc(sizeof(Ast));
  if (ast == NULL) {
    *d = (Diagnostic){
        .loc = {0}, .msg = "out of memory", .src = parser->srcName};
    return NULL;
  }

  *ast = (Ast){
      .type = NODE_LAMBDA,
      .val.lambda =
          (AstLambda){
              .param = param.val.s,
              .body = expr,
              .self = selfRef,
          },
      .loc =
          (Loc){
              .start = start,
              .end = expr->loc.end,
          },
  };

  return ast;
}

Ast *parsePrefix(Parser *parser, char c, Loc loc, Diagnostic *d) {
  UnOpType uot = toUot(c);

  if (uot == UNOP_NONE) {
    *d = (Diagnostic){
        .loc = loc, .msg = "expected prefix operator", .src = parser->srcName};
    return NULL;
  }

  Ast *expr = parseLeft(parser, PREFIX_BP, d);

  Ast *left = malloc(sizeof(Ast));
  if (left == NULL) {
    *d = (Diagnostic){
        .loc = {0}, .msg = "out of memory", .src = parser->srcName};
    return NULL;
  }

  *left = (Ast){
      .type = NODE_UNOP,
      .val.unop =
          (UnOp){
              .type = uot,
              .child = expr,
          },
      .loc = (Loc){.start = loc.start, .end = expr->loc.end},
  };
  return left;
}

Ast *parseLeft(Parser *parser, uint8_t minBp, Diagnostic *d) {
  Token t;
  Status s = getToken(parser, &t, d);

  if (s == ERR) {
    return NULL;
  }

  Ast *left;

  switch (t.type) {
  case TOKEN_EOF:
    *d = (Diagnostic){
        .loc = t.loc, .msg = "expected expression", .src = parser->srcName};
    return NULL;

  case TOKEN_LAMBDA:
    left = parseLambda(parser, d, t.loc.start);
    if (left == NULL) {
      return NULL;
    }
    break;

  case TOKEN_DOT:
    *d = (Diagnostic){
        .loc = t.loc, .msg = "unexpected dot operator", .src = parser->srcName};
    return NULL;

  case TOKEN_AT:
    *d = (Diagnostic){
        .loc = t.loc, .msg = "unexpected at operator", .src = parser->srcName};
    return NULL;

  case TOKEN_UNIT:
    left = malloc(sizeof(Ast));
    if (left == NULL) {
      *d = (Diagnostic){
          .loc = {0}, .msg = "out of memory", .src = parser->srcName};
      return NULL;
    }
    *left = (Ast){
        .type = NODE_UNIT,
        .val = {0},
        .loc = t.loc,
    };
    break;

  case TOKEN_LPAREN:
    Ast *inner = parseLeft(parser, 0, d);
    if (inner == NULL) {
      return NULL;
    }

    Token rparen;
    s = getToken(parser, &rparen, d);
    if (s == ERR) {
      *d = (Diagnostic){
          .loc = t.loc, .msg = "expected rparen", .src = parser->srcName};
      return NULL;
    }

    inner->loc.start = t.loc.start;
    inner->loc.end = rparen.loc.end;
    left = inner;
    break;

  case TOKEN_RPAREN:
    *d = (Diagnostic){
        .loc = t.loc, .msg = "unexpected rparen", .src = parser->srcName};
    return NULL;

  case TOKEN_INT:
    left = malloc(sizeof(Ast));
    if (left == NULL) {
      *d = (Diagnostic){
          .loc = {0}, .msg = "out of memory", .src = parser->srcName};
      return NULL;
    }
    *left = (Ast){
        .type = NODE_INT,
        .val.i = t.val.i,
        .loc = t.loc,
    };
    break;

  case TOKEN_OP:
    left = parsePrefix(parser, t.val.c, t.loc, d);
    if (left == NULL) {
      return NULL;
    }
    break;

  case TOKEN_SYM:
    left = malloc(sizeof(Ast));
    if (left == NULL) {
      *d = (Diagnostic){
          .loc = {0}, .msg = "out of memory", .src = parser->srcName};
      return NULL;
    }
    *left = (Ast){
        .type = NODE_SYM,
        .val.s = t.val.s,
        .loc = t.loc,
    };
    break;
  }

  return parseAdvance(parser, minBp, left, d);
}

Ast *parseAdvance(Parser *parser, uint8_t minBp, Ast *left, Diagnostic *d) {
  while (true) {
    Token t;
    Status s = getToken(parser, &t, d);
    if (s == ERR) {
      return NULL;
    }

    switch (t.type) {
    case TOKEN_EOF:
    case TOKEN_RPAREN: {
      pushToken(parser, t);
      return left;
    }

    case TOKEN_DOT:
    case TOKEN_AT: {
      *d = (Diagnostic){.loc = t.loc,
                        .msg = "unexpected dot operator",
                        .src = parser->srcName};
      return NULL;
    }

    case TOKEN_LAMBDA:
    case TOKEN_UNIT:
    case TOKEN_LPAREN:
    case TOKEN_INT:
    case TOKEN_SYM: {
      pushToken(parser, t);

      if (minBp >= APPLY_LEFT_BP) {
        return left;
      }

      Ast *right = parseLeft(parser, APPLY_RIGHT_BP, d);
      if (right == NULL) {
        return NULL;
      }

      Ast *new_left = malloc(sizeof(Ast));
      if (new_left == NULL) {
        *d = (Diagnostic){
            .loc = {0}, .msg = "out of memory", .src = parser->srcName};
        return NULL;
      }

      *new_left = (Ast){
          .type = NODE_BIOP,
          .val.biop =
              {
                  .type = BIOP_APPLY,
                  .left = left,
                  .right = right,
              },
          .loc = (Loc){.start = left->loc.start, .end = right->loc.end},
      };

      left = new_left;
      break;
    }

    case TOKEN_OP: {
      BiOpType bot = toBot(t.val.c);
      if (bot == BIOP_NONE) {
        *d = (Diagnostic){.loc = t.loc,
                          .msg = "unrecognized operator",
                          .src = parser->srcName};
        return NULL;
      }

      Bp bp = infixBp(t.val.c);
      if (minBp >= bp.left) {
        pushToken(parser, t);
        return left;
      }

      Ast *right = parseLeft(parser, bp.right, d);
      if (right == NULL) {
        return NULL;
      }

      Ast *new_left = malloc(sizeof(Ast));
      if (new_left == NULL) {
        *d = (Diagnostic){
            .loc = {0}, .msg = "out of memory", .src = parser->srcName};
        return NULL;
      }

      *new_left = (Ast){
          .type = NODE_BIOP,
          .val.biop = (BiOp){.type = bot, .left = left, .right = right},
          .loc = (Loc){.start = left->loc.start, .end = right->loc.end},
      };

      left = new_left;
      break;
    }
    }
  }
}

Ast *Parse(Parser *parser, Diagnostic *d) { return parseLeft(parser, 0, d); }
