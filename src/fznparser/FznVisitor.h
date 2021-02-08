#include "FlatZincBaseVisitor.h"
#include "Structure.h"

using namespace fznparser;

class FznVisitor : FlatZincBaseVisitor {
 public:
  antlrcpp::Any visitModel(FlatZincParser::ModelContext *ctx);
  antlrcpp::Any visitVarDeclItem(FlatZincParser::VarDeclItemContext *ctx);
  antlrcpp::Any visitConstraintItem(FlatZincParser::ConstraintItemContext *ctx);
  antlrcpp::Any visitBasicVarType(FlatZincParser::BasicVarTypeContext *ctx);
  antlrcpp::Any visitAnnotations(FlatZincParser::AnnotationsContext *ctx);
  antlrcpp::Any visitAnnotation(FlatZincParser::AnnotationContext *ctx);
  antlrcpp::Any visitExpr(FlatZincParser::ExprContext *ctx);
};
