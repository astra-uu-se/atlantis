#include "antlr4-runtime.h"
#include "FlatZincBaseVisitor.h"
#include "Structure.h"

using namespace fznparser;


class  FznVisitor : FlatZincBaseVisitor {
public:
  antlrcpp::Any visitModel(FlatZincParser::ModelContext *ctx);
  antlrcpp::Any visitParDeclItem(FlatZincParser::ParDeclItemContext *ctx);
  antlrcpp::Any visitVarDeclItem(FlatZincParser::VarDeclItemContext *ctx);
  antlrcpp::Any visitConstraintItem(FlatZincParser::ConstraintItemContext *ctx);
};

