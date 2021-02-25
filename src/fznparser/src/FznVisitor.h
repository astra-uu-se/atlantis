#pragma once
#include "FlatZincBaseVisitor.h"
#include "model.hpp"

using namespace fznparser;

class FznVisitor : FlatZincBaseVisitor {
 public:
  antlrcpp::Any visitModel(FlatZincParser::ModelContext *ctx);
  antlrcpp::Any visitVarDeclItem(FlatZincParser::VarDeclItemContext *ctx);
  antlrcpp::Any visitParDeclItem(FlatZincParser::ParDeclItemContext *ctx);
  antlrcpp::Any visitConstraintItem(FlatZincParser::ConstraintItemContext *ctx);
  antlrcpp::Any visitBasicVarType(FlatZincParser::BasicVarTypeContext *ctx);
  antlrcpp::Any visitAnnotations(FlatZincParser::AnnotationsContext *ctx);
  antlrcpp::Any visitAnnotation(FlatZincParser::AnnotationContext *ctx);
  antlrcpp::Any visitExpr(FlatZincParser::ExprContext *ctx);
  antlrcpp::Any visitArrayLiteral(FlatZincParser::ArrayLiteralContext *ctx);
  antlrcpp::Any visitParArrayLiteral(
      FlatZincParser::ParArrayLiteralContext *ctx);
  // antlrcpp::Any visitArrayVarType(FlatZincParser::ArrayVarTypeContext *ctx);
};
