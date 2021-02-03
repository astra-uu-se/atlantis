#include "FznVisitor.h"
#include <vector>

antlrcpp::Any FznVisitor::visitModel(FlatZincParser::ModelContext *ctx) {

  vector<Variable> variables;
  for (auto variable : ctx->varDeclItem()) {
    Variable v = visitVarDeclItem(variable);
    variables.push_back(v);
  }

  vector<Constraint> constraints;
  for (auto constraint : ctx->constraintItem()) {
    Constraint c = visitConstraintItem(constraint);
    constraints.push_back(c);
  }

  Model m = Model(variables, constraints);

  return m;
}


antlrcpp::Any FznVisitor::visitVarDeclItem(FlatZincParser::VarDeclItemContext *ctx) {
  string name = ctx->Identifier()->getText();
  vector<Annotation> annotations = visitAnnotations(ctx->annotations());
  // Normal Variable Declaration
  if (ctx->basicVarType()) {
    Domain domain = visitBasicVarType(ctx->basicVarType());
    return(Variable(name, domain, annotations));
  }
  // TODO: Array Variable Declaration
  return(Variable(name, Domain(), annotations));
}

antlrcpp::Any FznVisitor::visitConstraintItem(FlatZincParser::ConstraintItemContext *ctx) {
  string name = ctx->Identifier()->getText();

  return Constraint(name);
}

antlrcpp::Any FznVisitor::visitBasicVarType(FlatZincParser::BasicVarTypeContext *ctx) {
  return Domain();
}

antlrcpp::Any FznVisitor::visitAnnotations(FlatZincParser::AnnotationsContext *ctx) {
  vector<Annotation> annotations;
  annotations.push_back(Annotation());
  return annotations;
}
