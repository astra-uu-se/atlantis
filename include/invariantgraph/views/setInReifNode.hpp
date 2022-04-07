#pragma once

#include <utility>

#include "reifiedConstraint.hpp"

namespace invariantgraph {

class SetInReifNode : public ReifiedConstraint {
 public:
  static std::unique_ptr<SetInReifNode> fromModelConstraint(
      const std::shared_ptr<fznparser::Constraint>& constraint,
      const std::function<VariableNode*(std::shared_ptr<fznparser::Variable>)>&
          variableMap);

  SetInReifNode(std::unique_ptr<SoftConstraintNode> constraint, VariableNode* r)
      : ReifiedConstraint(std::move(constraint), r) {}
};

}  // namespace invariantgraph