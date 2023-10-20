#pragma once

#include <algorithm>
#include <fznparser/model.hpp>
#include <utility>

#include "invariantgraph/invariantGraph.hpp"
#include "invariantgraph/invariantNode.hpp"
#include "invariants/minSparse.hpp"

namespace invariantgraph {
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

  void registerOutputVariables(InvariantGraph&, Engine& engine) override;

  void registerNode(InvariantGraph&, Engine& engine) override;
};
}  // namespace invariantgraph
