#pragma once

#include "invariantgraph/structure.hpp"

namespace invariantgraph {

class ReifiedConstraint : public ViewNode {
 private:
  SoftConstraintNode* _constraint;
  VariableNode* _r;

 public:
  ReifiedConstraint(SoftConstraintNode* constraint, VariableNode* r)
      : ViewNode(nullptr, r->variable()), _constraint(constraint), _r(r) {}

  void registerWithEngine(Engine& engine,
                          std::map<VariableNode*, VarId>& map) override;

 protected:
  std::shared_ptr<View> createView(Engine &engine, VarId variable) const override;
};

}  // namespace invariantgraph
