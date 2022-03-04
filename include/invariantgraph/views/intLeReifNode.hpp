#pragma once

#include <utility>

#include "reifiedConstraint.hpp"

namespace invariantgraph {

class IntLeReifNode : public ReifiedConstraint {
 public:
  static std::unique_ptr<IntLeReifNode> fromModelConstraint(
      const std::shared_ptr<fznparser::Constraint>& constraint,
      const std::function<VariableNode*(std::shared_ptr<fznparser::Variable>)>&
          variableMap);

  IntLeReifNode(std::unique_ptr<SoftConstraintNode> constraint, std::shared_ptr<fznparser::SearchVariable> r)
      : ReifiedConstraint(std::move(constraint), std::move(r)) {}
};

}  // namespace invariantgraph