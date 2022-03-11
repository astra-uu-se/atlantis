#pragma once

#include <utility>

#include "reifiedConstraint.hpp"

namespace invariantgraph {

class IntLinNeReifNode : public ReifiedConstraint {
 public:
  static std::unique_ptr<IntLinNeReifNode> fromModelConstraint(
      const std::shared_ptr<fznparser::Constraint>& constraint,
      const std::function<VariableNode*(std::shared_ptr<fznparser::Variable>)>&
          variableMap);

  IntLinNeReifNode(std::unique_ptr<SoftConstraintNode> constraint, VariableNode* r)
      : ReifiedConstraint(std::move(constraint), r) {}
};

}  // namespace invariantgraph