#pragma once

#include "atlantis/invariantgraph/invariantNode.hpp"

namespace atlantis::invariantgraph {
class IntCountNode : public InvariantNode {
 private:
  propagation::VarViewId _intermediate{propagation::NULL_ID};
  Int _offset{0};
  Int _needle;

 public:
  IntCountNode(IInvariantGraph& graph,

               std::vector<VarNodeId>&& vars, Int needle, VarNodeId count);

  void init(InvariantNodeId) override;

  void updateState() override;

  void registerOutputVars() override;

  void registerNode() override;

  [[nodiscard]] const std::vector<VarNodeId>& haystack() const;

  [[nodiscard]] Int needle() const;

  virtual std::string dotLangIdentifier() const override;
};
}  // namespace atlantis::invariantgraph
