#ifndef symresolver_h
#define symresolver_h

#include "lib/ast.h"
#include "lib/diagnostic.h"
#include "lib/hashset.h"
#include "lib/hashtable.h"
#include <stdbool.h>

typedef struct {
  size_t param;
  void *body;
  Hashset *closure;
} Ir1Lambda;

typedef struct {
  NodeType type;
  union {
    int64_t i;
    size_t si;
    BiOp biop;
    UnOp unop;
    Ir1Lambda lambda;
  } val;
  Loc loc;
} Ir1;

typedef struct {
  char *srcName;
  Hashtable *symbols;
  size_t symCnt;
} SymResolver;

SymResolver *SymResolverCreate(char *srcName);
void SymResolverDestroy(SymResolver *analyzer);

Ir1 *SymResolverRun(SymResolver *analyzer, Ast *ast, Diagnostic *d);

#endif
