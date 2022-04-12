#pragma once

#include <fznparser/model.hpp>

#include "../structure.hpp"
#include "arrayIntElementNode.hpp"

namespace invariantgraph {

class ArrayBoolElementNode : public VariableDefiningNode {
 public:
  static std::unique_ptr<ArrayIntElementNode> fromModelConstraint(
      const fznparser::FZNModel& model, const fznparser::Constraint& constraint,
      const std::function<VariableNode*(MappableValue&)>& variableMap);
};

}  // namespace invariantgraph