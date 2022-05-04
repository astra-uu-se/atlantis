#pragma once

#include <fznparser/model.hpp>
#include <utility>

#include "invariantgraph/structure.hpp"

namespace invariantgraph {

class ReifiedConstraint : public VariableDefiningNode {
 protected:
  std::unique_ptr<SoftConstraintNode> _constraint;
  VariableNode* _r;

 public:
  ReifiedConstraint(std::unique_ptr<SoftConstraintNode> constraint,
                    VariableNode* r);

  void createDefinedVariables(
      Engine& engine, VariableDefiningNode::VariableMap& variableMap) override;

  void registerWithEngine(Engine& engine,
                          VariableDefiningNode::VariableMap& map) override;
};

}  // namespace invariantgraph
