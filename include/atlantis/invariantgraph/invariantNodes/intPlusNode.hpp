#pragma once

#include "atlantis/invariantgraph/invariantNode.hpp"

namespace atlantis::invariantgraph {

class IntPlusNode : public InvariantNode {
 private:
  Int _offset{0};
  propagation::VarViewId _intermediate{propagation::NULL_ID};

 public:
  IntPlusNode(IInvariantGraph& graph, VarNodeId a, VarNodeId b,
              VarNodeId output);

  void init(InvariantNodeId) override;

  void updateState() override;

  void registerOutputVars() override;

  void registerNode() override;

  bool canBeReplaced() const override;

  bool replace() override;

  [[nodiscard]] VarNodeId a() const noexcept {
    return staticInputVarNodeIds().front();
  }
  [[nodiscard]] VarNodeId b() const noexcept {
    return staticInputVarNodeIds().back();
  }

  virtual std::string dotLangIdentifier() const override;
};

}  // namespace atlantis::invariantgraph
