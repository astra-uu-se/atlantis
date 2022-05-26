#pragma once

#include <fznparser/model.hpp>

#include "arrayIntElementNode.hpp"
#include "invariantgraph/variableDefiningNode.hpp"

namespace invariantgraph {

class ArrayBoolElementNode : public VariableDefiningNode {
 public:
  static std::vector<std::pair<std::string_view, size_t>>
  acceptedNameNumArgPairs() {
    return std::vector<std::pair<std::string_view, size_t>>{
        {"array_bool_element", 3}};
  }

  static std::unique_ptr<ArrayIntElementNode> fromModelConstraint(
      const fznparser::FZNModel& model, const fznparser::Constraint& constraint,
      const std::function<VariableNode*(MappableValue&)>& variableMap);
};

}  // namespace invariantgraph