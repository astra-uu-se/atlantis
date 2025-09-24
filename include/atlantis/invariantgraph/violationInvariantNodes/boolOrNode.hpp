#pragma once

#include "atlantis/invariantgraph/violationInvariantNode.hpp"

namespace atlantis::invariantgraph {

class BoolOrNode : public ViolationInvariantNode {


 public:
  BoolOrNode(InvariantGraph& graph, VarNodeId a, VarNodeId b, VarNodeId r);

  BoolOrNode(InvariantGraph& graph, VarNodeId a, VarNodeId b,
             bool shouldHold = true);

  void init(InvariantNodeId) override;

  void registerOutputVars(propagation::SolverBase&, SolverMapping&) const override;

  void registerNode(propagation::SolverBase&, SolverMapping&) const override;

  [[nodiscard]] VarNodeId a() const noexcept {
    return staticInputVarNodeIds().front();
  }
  [[nodiscard]] VarNodeId b() const noexcept {
    return staticInputVarNodeIds().back();
  }

  [[nodiscard]] std::string dotLangIdentifier() const override;
};

}  // namespace atlantis::invariantgraph
