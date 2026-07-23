#include "lib/ast.h"
#include <stdlib.h>

void AstFree(Ast *ast) {
  switch (ast->type) {
  case NODE_LAMBDA:
    free(ast->val.lambda.param);
    free(ast->val.lambda.self);
    AstFree(ast->val.lambda.body);
    break;
  case NODE_BIOP:
    AstFree(ast->val.biop.left);
    AstFree(ast->val.biop.right);
    break;
  case NODE_UNOP:
    AstFree(ast->val.unop.child);
    break;
  default:
  }

  free(ast);
}
