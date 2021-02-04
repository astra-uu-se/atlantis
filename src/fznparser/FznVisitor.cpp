#include "FznVisitor.h"
#include <vector>

antlrcpp::Any FznVisitor::visitModel(FlatZincParser::ModelContext *ctx) {

  std::vector<std::shared_ptr<Variable>> variables;
  for (auto variable : ctx->varDeclItem()) {
    std::shared_ptr<Variable> v = visitVarDeclItem(variable);
    variables.push_back(v);
  }

  std::vector<std::shared_ptr<Constraint>> constraints;
  for (auto constraint : ctx->constraintItem()) {
    std::shared_ptr<Constraint> c = visitConstraintItem(constraint);
    constraints.push_back(c);
  }

  Model m = Model(variables, constraints);

  return m;
}


antlrcpp::Any FznVisitor::visitVarDeclItem(FlatZincParser::VarDeclItemContext *ctx) {
  std::string name = ctx->Identifier()->getText();
  std::vector<Annotation> annotations = visitAnnotations(ctx->annotations());

  if (ctx->basicVarType()) {
    std::shared_ptr<Domain> domain = visitBasicVarType(ctx->basicVarType());
    return(std::make_shared<Variable>(name, domain, annotations));
  }
  // TODO: Array Variable Declaration
  return(std::make_shared<Variable>(name, std::make_shared<IntDomain>(), annotations));
}

antlrcpp::Any FznVisitor::visitConstraintItem(FlatZincParser::ConstraintItemContext *ctx) {
  std::string name = ctx->Identifier()->getText();
  return std::make_shared<Constraint>(name);
}

antlrcpp::Any FznVisitor::visitBasicVarType(FlatZincParser::BasicVarTypeContext *ctx) {
  // Basic Types - No Domains
  if (ctx->basicParType()) {
    if (ctx->basicParType()->getText() == "bool") {
      std::shared_ptr<Domain> domain = std::make_shared<BoolDomain>();
      return domain;
    } if (ctx->basicParType()->getText() == "int") {
      std::shared_ptr<Domain> domain = std::make_shared<IntDomain>();
      return domain;
    }
  }

  if (ctx->intLiteral()[0]) {
    int lb = stoi(ctx->intLiteral()[0]->getText());
    int ub = stoi(ctx->intLiteral()[1]->getText());
    std::shared_ptr<Domain> domain = std::make_shared<IntDomain>(lb, ub);
    return domain;
  }

  std::cout << "Parsed unimplemented domain." << std::endl;
  return std::make_shared<IntDomain>();
}

antlrcpp::Any FznVisitor::visitAnnotations(FlatZincParser::AnnotationsContext *ctx) {
  std::vector<Annotation> annotations;
  annotations.push_back(Annotation());
  return annotations;
}
