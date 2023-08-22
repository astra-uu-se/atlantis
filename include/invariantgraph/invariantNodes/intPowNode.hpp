#pragma once

#include <cmath>

#include "invariantgraph/invariantGraph.hpp"
#include "invariantgraph/invariantNode.hpp"
#include "invariants/pow.hpp"

namespace invariantgraph {

class IntPowNode : public InvariantNode {
 public:
  IntPowNode(VarNodeId a, VarNodeId b, VarNodeId output)
      : InvariantNode({output}, {a, b}) {}

  ~IntPowNode() override = default;

  static std::vector<std::pair<std::string_view, size_t>>
  acceptedNameNumArgPairs() {
    return std::vector<std::pair<std::string_view, size_t>>{
        {"int_pow", 3}, {"int_pow_fixed", 3}};
  }

  static std::unique_ptr<IntPowNode> fromModelConstraint(
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