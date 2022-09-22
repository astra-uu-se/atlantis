#pragma once

#include <fznparser/model.hpp>

#include "invariantgraph/invariantNode.hpp"
#include "invariantgraph/invariants/arrayIntElement2dNode.hpp"
#include "invariants/element2dVar.hpp"

namespace invariantgraph {

class ArrayBoolElement2dNode : public InvariantNode {
 public:
  static std::vector<std::pair<std::string_view, size_t>>
  acceptedNameNumArgPairs() {
    return std::vector<std::pair<std::string_view, size_t>>{
        {"array_bool_element2d_nonshifted_flat", 7}};
  }

  static std::unique_ptr<ArrayIntElement2dNode> fromModelConstraint(
      const fznparser::FZNModel& model, const fznparser::Constraint& constraint,
      const std::function<VariableNode*(MappableValue&)>& variableMap);
};

}  // namespace invariantgraph