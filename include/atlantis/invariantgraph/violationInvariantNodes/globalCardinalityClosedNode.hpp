#pragma once

#include "atlantis/invariantgraph/violationInvariantNode.hpp"

namespace atlantis::invariantgraph {
class GlobalCardinalityClosedNode : public ViolationInvariantNode {
 private:
  std::vector<Int> _cover;

 public:
  explicit GlobalCardinalityClosedNode(IInvariantGraph& graph,
                                       std::vector<VarNodeId>&& inputs,
                                       std::vector<Int>&& cover,
                                       std::vector<VarNodeId>&& counts,
                                       VarNodeId r);

  explicit GlobalCardinalityClosedNode(IInvariantGraph& graph,
                                       std::vector<VarNodeId>&& inputs,
                                       std::vector<Int>&& cover,
                                       std::vector<VarNodeId>&& counts,
                                       bool shouldHold = true);

  void init(InvariantNodeId) override;

  void registerOutputVars() override;

  void registerNode() override;

  [[nodiscard]] bool canBeReplaced() const override;

  bool replace() override;
  virtual std::string dotLangIdentifier() const override;
};

}  // namespace atlantis::invariantgraph
