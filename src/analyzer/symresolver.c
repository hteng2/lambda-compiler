#include "analyzer/symresolver.h"
#include "lib/diagnostic.h"
#include "lib/hashset.h"
#include "lib/hashtable.h"
#include <stdint.h>
#include <string.h>

#define MAXLOAD 4

size_t strHash(void *str) {
  char *s = (char *)str;
  size_t hash = 5381;
  int c;

  while (true) {
    c = *s;
    if (c == '\0')
      break;
    s++;
    hash = ((hash << 5) + hash) + (size_t)c;
  }

  return hash;
}

bool strEq(void *str1, void *str2) { return strcmp(str1, str2) == 0; }

size_t szHash(void *n) { return (size_t)n; }

bool szEq(void *m, void *n) { return (size_t)m == (size_t)n; }

void freeIr1(Ir1 *ir1) {
  if (ir1 == NULL)
    return;

  switch (ir1->type) {
  case NODE_LAMBDA:
    HashsetDestroy(ir1->val.lambda.closure);
    freeIr1(ir1->val.lambda.body);
    break;
  case NODE_UNIT:
    break;
  case NODE_INT:
    break;
  case NODE_BIOP:
    freeIr1(ir1->val.biop.left);
    freeIr1(ir1->val.biop.right);
    break;
  case NODE_UNOP:
    freeIr1(ir1->val.unop.child);
    break;
  case NODE_SYM:
    break;
  }
  free(ir1);
}

SymResolver *SymResolverCreate(char *srcName) {
  SymResolver *analyzer = malloc(sizeof(SymResolver));
  if (analyzer == NULL) {
    return NULL;
  }

  Hashtable *symbols = HashtableCreate(1, MAXLOAD, &strHash, &strEq);
  if (symbols == NULL) {
    free(analyzer);
    return NULL;
  }

  *analyzer =
      (SymResolver){.srcName = srcName, .symbols = symbols, .symCnt = 0};
  return analyzer;
}

void SymResolverDestroy(SymResolver *analyzer) {
  HashtableDestroy(analyzer->symbols);
  free(analyzer);
}

Ir1 *srTrampoline(SymResolver *analyzer, Ast *ast, Hashset *closure,
                  Diagnostic *d);

Ir1 *srLambda(SymResolver *analyzer, Ast *ast, Hashset *closure,
              Diagnostic *d) {
  char *param = ast->val.lambda.param;
  size_t pi = analyzer->symCnt;
  Status s = HashtableAdd(analyzer->symbols, param, (void *)pi);
  if (s == ERR) {
    *d = (Diagnostic){
        .loc = {0}, .msg = "out of memory", .src = analyzer->srcName};
    return NULL;
  }
  analyzer->symCnt++;

  char *self = ast->val.lambda.self;
  bool hasSelf = self != NULL;
  size_t si = 0;
  if (hasSelf) {
    si = analyzer->symCnt;
    s = HashtableAdd(analyzer->symbols, self, (void *)si);
    if (s == ERR) {
      *d = (Diagnostic){
          .loc = {0}, .msg = "out of memory", .src = analyzer->srcName};
      return NULL;
    }
    analyzer->symCnt++;
  }

  Hashset *innerClosure = HashsetCreate(1, MAXLOAD, &szHash, &szEq);
  if (innerClosure == NULL) {
    *d = (Diagnostic){
        .loc = {0}, .msg = "out of memory", .src = analyzer->srcName};
    return NULL;
  }

  Ir1 *newBody = srTrampoline(analyzer, ast->val.lambda.body, innerClosure, d);
  if (newBody == NULL) {
    HashsetDestroy(innerClosure);
    return NULL;
  }

  s = HashsetMerge(innerClosure, closure);
  if (s == ERR) {
    HashsetDestroy(innerClosure);
    *d = (Diagnostic){
        .loc = {0}, .msg = "out of memory", .src = analyzer->srcName};
    return NULL;
  }

  HashsetRemove(closure, (void *)pi, NULL);
  HashtableRemove(analyzer->symbols, param, NULL);
  if (hasSelf) {
    HashsetRemove(closure, (void *)si, NULL);
    HashtableRemove(analyzer->symbols, self, NULL);
  }

  Ir1 *newLambda = malloc(sizeof(Ir1));
  if (newLambda == NULL) {
    HashsetDestroy(innerClosure);
    freeIr1(newBody);
    *d = (Diagnostic){
        .loc = {0}, .msg = "out of memory", .src = analyzer->srcName};
    return NULL;
  }

  *newLambda = (Ir1){.type = NODE_LAMBDA,
                     .val.lambda = {.param = pi,
                                    .hasSelf = hasSelf,
                                    .self = si,
                                    .body = newBody,
                                    .closure = closure},
                     .loc = ast->loc};

  return newLambda;
}

Ir1 *srUnit(SymResolver *analyzer, Ast *ast, Diagnostic *d) {
  Ir1 *newInt = malloc(sizeof(Ir1));
  if (newInt == NULL) {
    *d = (Diagnostic){
        .loc = {0}, .msg = "out of memory", .src = analyzer->srcName};
    return NULL;
  }

  *newInt = (Ir1){
      .type = NODE_UNIT,
      .val = {0},
      .loc = ast->loc,
  };

  return newInt;
}

Ir1 *srInt(SymResolver *analyzer, Ast *ast, Diagnostic *d) {
  int64_t i = ast->val.i;
  Ir1 *newInt = malloc(sizeof(Ir1));
  if (newInt == NULL) {
    *d = (Diagnostic){
        .loc = {0}, .msg = "out of memory", .src = analyzer->srcName};
    return NULL;
  }

  *newInt = (Ir1){
      .type = NODE_INT,
      .val.i = i,
      .loc = ast->loc,
  };

  return newInt;
}

Ir1 *srBiOp(SymResolver *analyzer, Ast *ast, Hashset *closure, Diagnostic *d) {
  Ir1 *newLeft = srTrampoline(analyzer, ast->val.biop.left, closure, d);
  if (newLeft == NULL) {
    return NULL;
  }

  Ir1 *newRight = srTrampoline(analyzer, ast->val.biop.right, closure, d);
  if (newRight == NULL) {
    freeIr1(newLeft);
    return NULL;
  }

  Ir1 *newBiOp = malloc(sizeof(Ir1));
  if (newBiOp == NULL) {
    freeIr1(newLeft);
    freeIr1(newRight);
    *d = (Diagnostic){
        .loc = {0}, .msg = "out of memory", .src = analyzer->srcName};
    return NULL;
  }

  *newBiOp = (Ir1){.type = NODE_BIOP,
                   .val.biop = {.type = ast->val.biop.type,
                                .left = newLeft,
                                .right = newRight},
                   .loc = ast->loc};

  return newBiOp;
}

Ir1 *srUnOp(SymResolver *analyzer, Ast *ast, Hashset *closure, Diagnostic *d) {
  Ir1 *newChild = srTrampoline(analyzer, ast->val.unop.child, closure, d);
  if (newChild == NULL) {
    return NULL;
  }

  Ir1 *newUnOp = malloc(sizeof(Ir1));
  if (newUnOp == NULL) {
    freeIr1(newChild);
    *d = (Diagnostic){
        .loc = {0}, .msg = "out of memory", .src = analyzer->srcName};
    return NULL;
  }

  *newUnOp = (Ir1){.type = NODE_UNOP,
                   .val.unop =
                       {
                           .type = ast->val.unop.type,
                           .child = newChild,
                       },
                   .loc = ast->loc};

  return newUnOp;
}

Ir1 *srSym(SymResolver *analyzer, Ast *ast, Hashset *closure, Diagnostic *d) {
  char *sym = ast->val.s;
  void **si = NULL; // (size_t as void*) pointer

  HashtableFind(analyzer->symbols, sym, &si);
  if (si == NULL) {
    *d = (Diagnostic){
        .loc = ast->loc, .msg = "undefined symbol", .src = analyzer->srcName};
    return NULL;
  }

  if (HashsetAdd(closure, *si) == ERR) {
    *d = (Diagnostic){
        .loc = {0}, .msg = "out of memory", .src = analyzer->srcName};
    return NULL;
  }

  Ir1 *newSym = malloc(sizeof(Ir1));
  if (newSym == NULL) {
    *d = (Diagnostic){
        .loc = {0}, .msg = "out of memory", .src = analyzer->srcName};
    return NULL;
  }

  *newSym = (Ir1){
      .type = NODE_SYM,
      .val.si = *(size_t *)si,
      .loc = ast->loc,
  };

  return newSym;
}

Ir1 *srTrampoline(SymResolver *analyzer, Ast *ast, Hashset *closure,
                  Diagnostic *d) {
  if (ast == NULL) {
    *d = (Diagnostic){
        .loc = {0}, .msg = "NULL ast node", .src = analyzer->srcName};
    return NULL;
  }

  switch (ast->type) {
  case NODE_LAMBDA:
    return srLambda(analyzer, ast, closure, d);
  case NODE_UNIT:
    return srUnit(analyzer, ast, d);
  case NODE_INT:
    return srInt(analyzer, ast, d);
  case NODE_BIOP:
    return srBiOp(analyzer, ast, closure, d);
  case NODE_UNOP:
    return srUnOp(analyzer, ast, closure, d);
  case NODE_SYM:
    return srSym(analyzer, ast, closure, d);
  }
}

Ir1 *SymResolverRun(SymResolver *analyzer, Ast *ast, Diagnostic *d) {
  if (ast == NULL) {
    *d = (Diagnostic){
        .loc = {0}, .msg = "NULL ast node", .src = analyzer->srcName};
    return NULL;
  }

  Hashset *closure = HashsetCreate(1, MAXLOAD, &szHash, &szEq);
  if (closure == NULL) {
    *d = (Diagnostic){
        .loc = {0}, .msg = "out of memory", .src = analyzer->srcName};
    return NULL;
  }

  return srTrampoline(analyzer, ast, closure, d);
}
