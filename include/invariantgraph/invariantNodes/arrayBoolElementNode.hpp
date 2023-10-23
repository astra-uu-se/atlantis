#pragma once

#include <fznparser/model.hpp>

#include "invariantgraph/invariantGraph.hpp"
#include "invariantgraph/invariantNode.hpp"
#include "invariantgraph/invariantNodes/arrayIntElementNode.hpp"

namespace atlantis::invariantgraph {

class ArrayBoolElementNode : public InvariantNode {
 public:
  static std::vector<std::pair<std::string, size_t>> acceptedNameNumArgPairs() {
    return std::vector<std::pair<std::string, size_t>>{
        {"array_bool_element", 3}, {"array_bool_element_offset", 4}};
  }

  static std::unique_ptr<ArrayIntElementNode> fromModelConstraint(
      const fznparser::Constraint&, InvariantGraph&);
};

}  // namespace invariantgraph