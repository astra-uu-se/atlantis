#include "antlr4-runtime.h"
#include "FlatZincBaseVisitor.h"

using namespace fznparser;


class  FznVisitor : FlatZincBaseVisitor {
public:
  antlrcpp::Any visitModel(FlatZincParser::ModelContext *ctx);
  antlrcpp::Any visitConstraintItem(FlatZincParser::ConstraintItemContext *ctx);
};

