#pragma once

#include <fznparser/model.hpp>
#include <utility>

#include "invariantgraph/structure.hpp"

namespace invariantgraph {

class ReifiedConstraint : public VariableDefiningNode {
 private:
  std::unique_ptr<SoftConstraintNode> _constraint;
  VariableNode* _r;

 public:
  ReifiedConstraint(std::unique_ptr<SoftConstraintNode> constraint,
                    VariableNode* r);

  void registerWithEngine(Engine& engine,
                          VariableDefiningNode::VariableMap& map) override;
};

}  // namespace invariantgraph
