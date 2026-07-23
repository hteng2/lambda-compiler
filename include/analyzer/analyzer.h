#ifndef analyzer_h
#define analyzer_h

#include "lib/ast.h"
#include "lib/diagnostic.h"
#include "symresolver.h"
#include <stdbool.h>

typedef Ir1 Ir;

typedef struct {
  char *srcName;
} Analyzer;

Analyzer *AnalyzerCreate(char *srcName);
void AnalyzerDestroy(Analyzer *analyzer);

Ir *AnalyzerRun(Analyzer *analyzer, Ast *ast, Diagnostic *d);

#endif
