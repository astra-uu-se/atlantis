#pragma once

#include <fznparser/model.hpp>

#include "invariantgraph/invariantGraph.hpp"
#include "invariantgraph/invariants/arrayIntElementNode.hpp"
#include "invariantgraph/variableDefiningNode.hpp"

namespace invariantgraph {

class ArrayBoolElementNode : public VariableDefiningNode {
 public:
  static std::vector<std::pair<std::string_view, size_t>>
  acceptedNameNumArgPairs() {
    return std::vector<std::pair<std::string_view, size_t>>{
        {"array_bool_element", 3}, {"array_bool_element_offset", 4}};
  }

  static std::unique_ptr<ArrayIntElementNode> fromModelConstraint(
      const fznparser::Model& model, const fznparser::Constraint& constraint,
      InvariantGraph& invariantGraph);
};

}  // namespace invariantgraph