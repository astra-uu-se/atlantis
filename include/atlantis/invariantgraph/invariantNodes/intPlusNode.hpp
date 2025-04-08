#pragma once

#include "atlantis/invariantgraph/invariantNode.hpp"

namespace atlantis::invariantgraph {

class IntPlusNode : public InvariantNode {
  Int _offset{0};
  propagation::VarViewId _intermediate{propagation::NULL_ID};

 public:
  IntPlusNode(InvariantGraph& graph, VarNodeId a, VarNodeId b,
              VarNodeId output);

  void init(InvariantNodeId) override;

  void updateState() override;

  void registerOutputVars() override;

  void registerNode() override;

  [[nodiscard]] bool canBeReplaced() const override;

  bool replace() override;

  [[nodiscard]] VarNodeId a() const noexcept {
    return staticInputVarNodeIds().front();
  }
  [[nodiscard]] VarNodeId b() const noexcept {
    return staticInputVarNodeIds().back();
  }

  [[nodiscard]] std::string dotLangIdentifier() const override;
};

}  // namespace atlantis::invariantgraph
