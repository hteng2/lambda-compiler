#include "analyzer/analyzer.h"
#include "lexer/lexer.h"
#include "lib/ast.h"
#include "lib/diagnostic.h"
#include "lib/hashset.h"
#include "lib/token.h"
#include "parser/parser.h"
#include <error.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void printLoc(Loc loc) {
  char buff[16];
  snprintf(buff, 16, "%lu:%lu-%lu:%lu", loc.start.row, loc.start.col,
           loc.end.row, loc.end.col);
  size_t l = strlen(buff);
  for (size_t i = 0; i < 16 - l; i++) {
    printf(" ");
  }
  printf("%s", buff);
}
void printUsage(void) { printf("usage: lambda <file>\n"); }

void printDiagnostic(Diagnostic d) {
  printf("%s ", d.src);
  printLoc(d.loc);
  printf("%s\n", d.msg);
}

void printToken(Token t) {
  printLoc(t.loc);
  switch (t.type) {
  case TOKEN_EOF:
    printf("EOF\n");
    return;
  case TOKEN_LAMBDA:
    printf("LAMBDA\n");
    return;
  case TOKEN_DOT:
    printf("DOT\n");
    return;
  case TOKEN_UNIT:
    printf("UNIT\n");
    return;
  case TOKEN_LPAREN:
    printf("LPAREN\n");
    return;
  case TOKEN_RPAREN:
    printf("RPAREN\n");
    return;
  case TOKEN_INT:
    printf("INT %ld\n", t.val.i);
    return;
  case TOKEN_OP:
    printf("OP %c\n", t.val.c);
    return;
  case TOKEN_SYM:
    printf("SYM %s\n", t.val.s);
    return;
  }
}

void botToStr(BiOpType bot, char *s, size_t len) {
  switch (bot) {
  case BIOP_NONE:
    strncpy(s, "none", len);
    break;
  case BIOP_APPLY:
    strncpy(s, "apply", len);
    break;
  case BIOP_ADD:
    strncpy(s, "add", len);
    break;
  case BIOP_SUB:
    strncpy(s, "sub", len);
    break;
  case BIOP_MUL:
    strncpy(s, "mul", len);
    break;
  case BIOP_DIV:
    strncpy(s, "div", len);
    break;
  case BIOP_MOD:
    strncpy(s, "mod", len);
    break;
  }
}

void uotToStr(UnOpType uot, char *s, size_t len) {
  switch (uot) {
  case UNOP_NONE:
    strncpy(s, "none", len);
    break;
  case UNOP_NEG:
    strncpy(s, "neg", len);
    break;
  }
}

void printAst(Ast *ast, uint8_t lvl) {
  printLoc(ast->loc);
  printf("|");

  for (uint8_t i = 0; i < lvl; i++) {
    printf("  ");
  }

  size_t len = 16;
  char buff[len];

  switch (ast->type) {
  case NODE_LAMBDA:
    printf("lambda %s\n", ast->val.lambda.param);
    printAst(ast->val.lambda.body, lvl + 1);
    return;

  case NODE_UNIT:
    printf("unit\n");
    return;

  case NODE_INT:
    printf("int %ld\n", ast->val.i);
    return;

  case NODE_BIOP:
    botToStr(ast->val.biop.type, buff, len - 1);
    buff[len - 1] = '\0';
    printf("biop %s\n", buff);
    printAst(ast->val.biop.left, lvl + 1);
    printAst(ast->val.biop.right, lvl + 1);
    return;

  case NODE_UNOP:
    uotToStr(ast->val.unop.type, buff, len - 1);
    buff[len - 1] = '\0';
    printf("unop %s\n", buff);
    printAst(ast->val.unop.child, lvl + 1);
    return;

  case NODE_SYM:
    printf("sym %s\n", ast->val.s);
    return;
  }
}

void printClosureSym(void *key) { printf("%ld", (size_t)key); }

void printIr1(Ir1 *ir1, uint8_t lvl) {
  printLoc(ir1->loc);
  printf("|");

  for (uint8_t i = 0; i < lvl; i++) {
    printf("  ");
  }

  size_t len = 16;
  char buff[len];

  switch (ir1->type) {
  case NODE_LAMBDA:
    printf("lambda %ld: ", ir1->val.lambda.param);
    HashsetIter(ir1->val.lambda.closure, &printClosureSym);
    printf("\n");
    printIr1(ir1->val.lambda.body, lvl + 1);
    return;

  case NODE_UNIT:
    printf("unit\n");
    return;

  case NODE_INT:
    printf("int %ld\n", ir1->val.i);
    return;

  case NODE_BIOP:
    botToStr(ir1->val.biop.type, buff, len - 1);
    buff[len - 1] = '\0';
    printf("biop %s\n", buff);
    printIr1(ir1->val.biop.left, lvl + 1);
    printIr1(ir1->val.biop.right, lvl + 1);
    return;

  case NODE_UNOP:
    uotToStr(ir1->val.unop.type, buff, len - 1);
    buff[len - 1] = '\0';
    printf("unop %s\n", buff);
    printIr1(ir1->val.unop.child, lvl + 1);
    return;

  case NODE_SYM:
    printf("sym %ld\n", ir1->val.si);
    return;
  }
}

int main(int argc, char **argv) {
  if (argc != 2) {
    printUsage();
    return 0;
  }

  char *srcName = argv[1];

  FILE *srcFile = fopen(srcName, "r");
  if (srcFile == NULL) {
    printf("failed to open file\n");
    return 0;
  }

  Diagnostic d;
  Lexer lexer = LexerCreate(srcName, srcFile);
  Parser parser = ParserCreate(&lexer);

  Ast *ast = Parse(&parser, &d);
  fclose(srcFile);
  if (ast == NULL) {
    printDiagnostic(d);
    return 0;
  }

  Analyzer *analyzer = AnalyzerCreate(srcName);
  Ir1 *ir1 = AnalyzerRun(analyzer, ast, &d);
  AstFree(ast);
  if (ir1 == NULL) {
    printDiagnostic(d);
    return 0;
  }

  printIr1(ir1, 0);

  return 0;
}
