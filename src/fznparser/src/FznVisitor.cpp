#include "FznVisitor.h"
#include <vector>

antlrcpp::Any FznVisitor::visitModel(FlatZincParser::ModelContext *ctx) {
  Model m = Model();
  for (auto item : ctx->varDeclItem()) {
      m.addVariable(visitVarDeclItem(item));
  }
  for (auto constraintItem : ctx->constraintItem()) {
    m.addConstraint(visitConstraintItem(constraintItem));
  }
  return m;
}

antlrcpp::Any FznVisitor::visitVarDeclItem(
    FlatZincParser::VarDeclItemContext *ctx) {
  std::string name = ctx->Identifier()->getText();
  std::vector<Annotation> annotations = visitAnnotations(ctx->annotations());

  if (ctx->basicVarType()) {
    std::shared_ptr<Domain> domain = visitBasicVarType(ctx->basicVarType());
    return static_cast<std::shared_ptr<Variable>>(std::make_shared<SingleVariable>(name, annotations, domain));
  } else {//if (ctx->arrayVarType()) {
    std::vector<Expression> elements = visitArrayLiteral(ctx->arrayLiteral());
    return static_cast<std::shared_ptr<Variable>>(std::make_shared<ArrayVariable>(name, annotations, elements));
  }
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
  return ConstraintBox(name, expressions,
                        visitAnnotations(ctx->annotations()));
}

antlrcpp::Any FznVisitor::visitExpr(FlatZincParser::ExprContext *ctx) {
  if (auto b = ctx->basicExpr()) {
    if (b->Identifier()) {
      return Expression(b->Identifier()->getText(), true);
    } else if (auto ble = b->basicLiteralExpr()) {
      return Expression(ble->getText(), false);
    }
  } else if (auto c = ctx->arrayLiteral()) {
    return Expression(c->getText(), visitArrayLiteral(c), false);
  }
  std::cout << "Parsed something wrong" << std::endl;
  return Expression();
}


antlrcpp::Any FznVisitor::visitArrayLiteral(
    FlatZincParser::ArrayLiteralContext *ctx) {
  std::vector<Expression> ab;
  for (auto c : ctx->basicExpr()) {
    if (c->Identifier()) {
      ab.push_back(Expression(c->Identifier()->getText(), true));
    } else {
      ab.push_back(Expression(c->getText(), false));
    }
  }
  return ab;
}
