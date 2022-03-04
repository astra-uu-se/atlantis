#pragma once

#include <utility>

#include "reifiedConstraint.hpp"

namespace invariantgraph {

class IntEqReifNode : public ReifiedConstraint {
 public:
  static std::unique_ptr<IntEqReifNode> fromModelConstraint(
      const std::shared_ptr<fznparser::Constraint>& constraint,
      const std::function<VariableNode*(std::shared_ptr<fznparser::Variable>)>&
          variableMap);

  IntEqReifNode(std::unique_ptr<SoftConstraintNode> constraint, std::shared_ptr<fznparser::SearchVariable> r)
      : ReifiedConstraint(std::move(constraint), std::move(r)) {}
};

}  // namespace invariantgraph