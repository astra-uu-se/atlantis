#pragma once

#include <fznparser/model.hpp>
#include <utility>

#include "reifiedConstraint.hpp"

namespace invariantgraph {

class IntLinEqReifNode : public ReifiedConstraint {
 public:
  static std::unique_ptr<IntLinEqReifNode> fromModelConstraint(
      const fznparser::FZNModel& model, const fznparser::Constraint& constraint,
      const std::function<VariableNode*(MappableValue&)>& variableMap);

  IntLinEqReifNode(std::unique_ptr<SoftConstraintNode> constraint,
                   VariableNode* r)
      : ReifiedConstraint(std::move(constraint), r) {}
};

}  // namespace invariantgraph