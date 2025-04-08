#pragma once

#include "atlantis/invariantgraph/violationInvariantNode.hpp"

namespace atlantis::invariantgraph {
class SetInNode : public ViolationInvariantNode {
  std::vector<Int> _values;
  propagation::VarViewId _intermediate{propagation::NULL_ID};

 public:
  explicit SetInNode(InvariantGraph& graph, VarNodeId input,
                     std::vector<Int>&& values, VarNodeId r);

  explicit SetInNode(InvariantGraph& graph, VarNodeId input,
                     std::vector<Int>&& values, bool shouldHold = true);

  void init(InvariantNodeId) override;

  void registerOutputVars() override;

  void registerNode() override;

  [[nodiscard]] const std::vector<Int>& values() { return _values; }

  [[nodiscard]] std::string dotLangIdentifier() const override;
};
}  // namespace atlantis::invariantgraph
