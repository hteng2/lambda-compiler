#include "analyzer/analyzer.h"
#include "analyzer/symresolver.h"
#include <stdint.h>
#include <string.h>

Analyzer *AnalyzerCreate(char *srcName) {
  Analyzer *analyzer = malloc(sizeof(Analyzer));
  if (analyzer == NULL) {
    return NULL;
  }

  *analyzer = (Analyzer){.srcName = srcName};

  return analyzer;
}
void AnalyzerDestroy(Analyzer *analyzer) { free(analyzer); }

Ir *AnalyzerRun(Analyzer *analyzer, Ast *ast, Diagnostic *d) {
  SymResolver *sr = SymResolverCreate(analyzer->srcName);
  if (sr == NULL) {
    *d = (Diagnostic){
        .loc = {0}, .msg = "out of memory", .src = analyzer->srcName};
    return NULL;
  }

  Ir1 *ir1 = SymResolverRun(sr, ast, d);
  if (ir1 == NULL) {
    return NULL;
  }

  return ir1;
}
