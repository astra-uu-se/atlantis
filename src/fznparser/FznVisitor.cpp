#include "FznVisitor.h"

antlrcpp::Any FznVisitor::visitModel(FlatZincParser::ModelContext *ctx) {
  visitChildren(ctx);
  return 7;
}


antlrcpp::Any FznVisitor::visitConstraintItem(FlatZincParser::ConstraintItemContext *ctx) {
  std::cout << ctx->Identifier()->getText() << std::endl;
  return visitChildren(ctx);
}
