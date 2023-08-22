#pragma once

#include <cmath>

#include "invariantgraph/invariantGraph.hpp"
#include "invariantgraph/invariantNode.hpp"
#include "invariants/binaryMax.hpp"

namespace invariantgraph {

class IntMaxNode : public InvariantNode {
 public:
  IntMaxNode(VarNodeId a, VarNodeId b, VarNodeId output)
      : InvariantNode({output}, {a, b}) {}

  ~IntMaxNode() override = default;

  static std::vector<std::pair<std::string_view, size_t>>
  acceptedNameNumArgPairs() {
    return std::vector<std::pair<std::string_view, size_t>>{{"int_max", 3}};
  }

  static std::unique_ptr<IntMaxNode> fromModelConstraint(
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