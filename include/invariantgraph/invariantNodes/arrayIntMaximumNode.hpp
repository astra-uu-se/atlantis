#pragma once

#include <algorithm>
#include <fznparser/model.hpp>
#include <utility>

#include "invariantgraph/invariantGraph.hpp"
#include "invariantgraph/invariantNode.hpp"
#include "propagation/invariants/maxSparse.hpp"

namespace atlantis::invariantgraph {
class ArrayIntMaximumNode : public InvariantNode {
 public:
  ArrayIntMaximumNode(std::vector<VarNodeId>&& variables, VarNodeId output);

  static std::vector<std::pair<std::string, size_t>> acceptedNameNumArgPairs() {
    return std::vector<std::pair<std::string, size_t>>{
        {"array_int_maximum", 2}};
  }

  static std::unique_ptr<ArrayIntMaximumNode> fromModelConstraint(
      const fznparser::Constraint&, InvariantGraph&);

  ~ArrayIntMaximumNode() override = default;

  void registerOutputVariables(InvariantGraph&, propagation::Engine& engine) override;

  void registerNode(InvariantGraph&, propagation::Engine& engine) override;
};
}  // namespace invariantgraph
