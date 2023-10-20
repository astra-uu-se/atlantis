#pragma once

#include <algorithm>
#include <fznparser/model.hpp>
#include <utility>

#include "invariantgraph/invariantGraph.hpp"
#include "invariantgraph/invariantNode.hpp"
#include "invariants/maxSparse.hpp"

namespace invariantgraph {
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

  void registerOutputVariables(InvariantGraph&, Engine& engine) override;

  void registerNode(InvariantGraph&, Engine& engine) override;
};
}  // namespace invariantgraph
