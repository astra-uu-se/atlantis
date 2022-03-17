#pragma once

#include <utility>

#include "invariantgraph/structure.hpp"

namespace invariantgraph {

class ReifiedConstraint : public VariableDefiningNode {
 private:
  std::unique_ptr<SoftConstraintNode> _constraint;
  VariableNode* _r;

 public:
  ReifiedConstraint(std::unique_ptr<SoftConstraintNode> constraint, VariableNode* r)
      : VariableDefiningNode({r}), _constraint(std::move(constraint)), _r(r) {}

  void registerWithEngine(Engine& engine,
                          std::map<VariableNode*, VarId>& map) override;
};

}  // namespace invariantgraph
