#pragma once

#include <cmath>

#include "invariantgraph/invariantGraph.hpp"
#include "invariantgraph/invariantNode.hpp"
#include "invariants/binaryMin.hpp"

namespace invariantgraph {

class IntMinNode : public InvariantNode {
 public:
  IntMinNode(VarNodeId a, VarNodeId b, VarNodeId output)
      : InvariantNode({output}, {a, b}) {}

  ~IntMinNode() override = default;

  static std::vector<std::pair<std::string_view, size_t>>
  acceptedNameNumArgPairs() {
    return std::vector<std::pair<std::string_view, size_t>>{{"int_min", 3}};
  }

  static std::unique_ptr<IntMinNode> fromModelConstraint(
      const fznparser::Constraint&, InvariantGraph&);

  void registerOutputVariables(InvariantGraph&, Engine& engine) override;

  void registerNode(InvariantGraph&, Engine& engine) override;

  [[nodiscard]] VarNodeId a() const noexcept {
    return staticInputVarNodeIds().front();
  }
  [[nodiscard]] VarNodeId b() const noexcept {
    return staticInputVarNodeIds().back();
  }
};

}  // namespace invariantgraph