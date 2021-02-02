#include "FznVisitor.h"
#include <vector>

antlrcpp::Any FznVisitor::visitModel(FlatZincParser::ModelContext *ctx) {

  vector<Parameter> parameters;
  for (auto parameter : ctx->parDeclItem()) {
    Parameter p = visitParDeclItem(parameter);
    parameters.push_back(p);
  }

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

  Model m = Model(parameters, variables, constraints);

  return m;
}


antlrcpp::Any FznVisitor::visitParDeclItem(FlatZincParser::ParDeclItemContext *ctx) {
  string name = ctx->Identifier()->getText();

  return Parameter(name);
}
antlrcpp::Any FznVisitor::visitVarDeclItem(FlatZincParser::VarDeclItemContext *ctx) {
  string name = ctx->Identifier()->getText();

  return Variable(name);
}
antlrcpp::Any FznVisitor::visitConstraintItem(FlatZincParser::ConstraintItemContext *ctx) {
  string name = ctx->Identifier()->getText();

  return Constraint(name);
}
