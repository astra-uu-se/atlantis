#pragma once

#include "atlantis/invariantgraph/violationInvariantNode.hpp"

namespace atlantis::invariantgraph {
class BoolClauseNode : public ViolationInvariantNode {
 private:
  size_t _numAs;
  propagation::VarViewId _sumVarId{propagation::NULL_ID};

 public:
  explicit BoolClauseNode(IInvariantGraph& graph, std::vector<VarNodeId>&& as,
                          std::vector<VarNodeId>&& bs, VarNodeId r);

  explicit BoolClauseNode(IInvariantGraph& graph, std::vector<VarNodeId>&& as,
                          std::vector<VarNodeId>&& bs, bool shouldHold = true);

  void init(InvariantNodeId) override;

  void updateState() override;

  [[nodiscard]] bool canBeReplaced() const override;

  [[nodiscard]] bool replace() override;

  void registerOutputVars() override;

  void registerNode() override;
  virtual std::string dotLangIdentifier() const override;
};
}  // namespace atlantis::invariantgraph
