#pragma once

#include <algorithm>
#include <fznparser/model.hpp>
#include <utility>

#include "invariantgraph/invariantGraph.hpp"
#include "invariantgraph/invariantNode.hpp"
#include "propagation/invariants/minSparse.hpp"

namespace atlantis::invariantgraph {
class ArrayIntMinimumNode : public InvariantNode {
 public:
  ArrayIntMinimumNode(std::vector<VarNodeId>&& variables, VarNodeId output);

  ~ArrayIntMinimumNode() override = default;

  static std::vector<std::pair<std::string, size_t>> acceptedNameNumArgPairs() {
    return std::vector<std::pair<std::string, size_t>>{
        {"array_int_minimum", 2}};
  }

  static std::unique_ptr<ArrayIntMinimumNode> fromModelConstraint(
      const fznparser::Constraint&, InvariantGraph&);

  void registerOutputVariables(InvariantGraph&, propagation::Engine& engine) override;

  void registerNode(InvariantGraph&, propagation::Engine& engine) override;
};
}  // namespace invariantgraph
