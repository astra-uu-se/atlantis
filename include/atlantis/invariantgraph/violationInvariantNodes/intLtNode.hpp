#pragma once

#include "atlantis/invariantgraph/violationInvariantNode.hpp"

namespace atlantis::invariantgraph {

class IntLtNode : public ViolationInvariantNode {
 public:
  IntLtNode(IInvariantGraph& graph, VarNodeId a, VarNodeId b, VarNodeId r);

  IntLtNode(IInvariantGraph& graph, VarNodeId a, VarNodeId b,
            bool shouldHold = true);

  void init(InvariantNodeId) override;

  void updateState() override;

  void registerOutputVars() override;

  void registerNode() override;

  [[nodiscard]] VarNodeId a() const noexcept {
    return staticInputVarNodeIds().front();
  }
  [[nodiscard]] VarNodeId b() const noexcept {
    return staticInputVarNodeIds().back();
  }

  virtual std::string dotLangIdentifier() const override;
};

}  // namespace atlantis::invariantgraph
