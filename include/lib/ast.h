#ifndef ast_h
#define ast_h

#include "loc.h"

typedef enum {
  NODE_LAMBDA,
  NODE_INT,
  NODE_UNIT,
  NODE_BIOP,
  NODE_UNOP,
  NODE_SYM,
} NodeType;

typedef enum {
  BIOP_NONE,
  BIOP_APPLY,
  BIOP_ADD,
  BIOP_SUB,
  BIOP_MUL,
  BIOP_DIV,
  BIOP_MOD,
} BiOpType;

typedef struct {
  BiOpType type;
  void *left;
  void *right;
} BiOp;

typedef enum { UNOP_NONE, UNOP_NEG } UnOpType;

typedef struct {
  UnOpType type;
  void *child;
} UnOp;

typedef struct {
  char *param;
  void *body;
} AstLambda;

typedef struct {
  NodeType type;
  union {
    int64_t i;
    char *s;
    BiOp biop;
    UnOp unop;
    AstLambda lambda;
  } val;
  Loc loc;
} Ast;

/**
 * frees everything in the tree
 * therefore values cannot be moved, must be copied
 */
void AstFree(Ast *ast);

#endif
