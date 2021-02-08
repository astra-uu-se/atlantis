#include "FznVisitor.h"

#include <vector>

antlrcpp::Any FznVisitor::visitModel(FlatZincParser::ModelContext *ctx) {
  std::map<std::string, std::shared_ptr<Variable>> variables;
  for (auto variable : ctx->varDeclItem()) {
    std::shared_ptr<Variable> v = visitVarDeclItem(variable);
    variables.insert(
        std::pair<std::string, std::shared_ptr<Variable>>(v->_name, v));
  }

  std::vector<std::shared_ptr<Constraint>> constraints;
  for (auto ci : ctx->constraintItem()) {
    constraints.push_back(
        Model::createConstraint(visitConstraintItem(ci), variables));
  }

  Model m = Model(variables, constraints);
  return m;
}

antlrcpp::Any FznVisitor::visitVarDeclItem(
    FlatZincParser::VarDeclItemContext *ctx) {
  std::string name = ctx->Identifier()->getText();
  std::vector<Annotation> annotations = visitAnnotations(ctx->annotations());

  if (ctx->basicVarType()) {
    std::shared_ptr<Domain> domain = visitBasicVarType(ctx->basicVarType());
    return (std::make_shared<Variable>(name, domain, annotations));
  }
  // TODO: Array Variable Declaration
  return (std::make_shared<Variable>(name, std::make_shared<IntDomain>(),
                                     annotations));
}

antlrcpp::Any FznVisitor::visitBasicVarType(
    FlatZincParser::BasicVarTypeContext *ctx) {
  if (ctx->basicParType()) {
    if (ctx->basicParType()->getText() == "bool") {
      return static_cast<std::shared_ptr<Domain>>(
          std::make_shared<BoolDomain>());
    }
    if (ctx->basicParType()->getText() == "int") {
      return static_cast<std::shared_ptr<Domain>>(
          std::make_shared<IntDomain>());
    }
  }

  if (ctx->intRange()) {
    int lb = stoi(ctx->intRange()->intLiteral()[0]->getText());
    int ub = stoi(ctx->intRange()->intLiteral()[1]->getText());
    return static_cast<std::shared_ptr<Domain>>(
        std::make_shared<IntDomain>(lb, ub));
  }

  if (ctx->set()) {
    std::set<int> s;
    for (auto i : ctx->set()->intLiteral()) {
      s.insert(stoi(i->getText()));
    }
    // TODO: Verify move.
    return static_cast<std::shared_ptr<Domain>>(std::make_shared<IntDomain>(s));
  }

  std::cout << "Parsed unimplemented domain." << std::endl;
  return std::make_shared<IntDomain>();
}

antlrcpp::Any FznVisitor::visitAnnotations(
    FlatZincParser::AnnotationsContext *ctx) {
  std::vector<Annotation> annotations;
  for (auto a : ctx->annotation()) {
    annotations.push_back(visitAnnotation(a));
  }
  return annotations;
}

antlrcpp::Any FznVisitor::visitAnnotation(
    // TODO: Get rest of annotation info.
    FlatZincParser::AnnotationContext *ctx) {
  return Annotation(ctx->Identifier()->getText());
}

antlrcpp::Any FznVisitor::visitConstraintItem(
    FlatZincParser::ConstraintItemContext *ctx) {
  std::string name = ctx->Identifier()->getText();
  std::vector<Expression> expressions;
  for (auto exp : ctx->expr()) {
    expressions.push_back(visitExpr(exp));
  }
  return ConstraintItem(name, expressions,
                        visitAnnotations(ctx->annotations()));
}

antlrcpp::Any FznVisitor::visitExpr(FlatZincParser::ExprContext *ctx) {
  if (auto b = ctx->basicExpr()) {
    if (b->Identifier()) return Expression(b->Identifier()->getText(), true);
  }
  return Expression();
}
