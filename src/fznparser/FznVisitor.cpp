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

antlrcpp::Any FznVisitor::visitConstraintItem(
    FlatZincParser::ConstraintItemContext *ctx) {
  std::string name = ctx->Identifier()->getText();
  return std::make_shared<Constraint>(name);
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
  // TODO: Är detta okej eller ska jag använda make_shared?
    std::set<int> s;
    for (auto i : ctx->set()->intLiteral()) {
      s.insert(stoi(i->getText()));
    }
    return static_cast<std::shared_ptr<Domain>>(std::make_shared<IntDomain>(s));
  }

  std::cout << "Parsed unimplemented domain." << std::endl;
  return std::make_shared<IntDomain>();
}

antlrcpp::Any FznVisitor::visitAnnotations(
    FlatZincParser::AnnotationsContext *ctx) {
  // TODO: Är detta okej eller ska jag använda make_shared?
  std::vector<Annotation> annotations;
  for (auto a : ctx->annotation()) {
    annotations.push_back(visitAnnotation(a));
  }
  return annotations;
}

antlrcpp::Any FznVisitor::visitAnnotation(
    FlatZincParser::AnnotationContext *ctx) {
  return Annotation(ctx->Identifier()->getText());
}
