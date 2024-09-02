#pragma once

#include "atlantis/invariantgraph/violationInvariantNode.hpp"

namespace atlantis::invariantgraph {
class SetInNode : public ViolationInvariantNode {
 private:
  std::vector<Int> _values;
  propagation::VarViewId _intermediate{propagation::NULL_ID};

 public:
  explicit SetInNode(IInvariantGraph& graph, VarNodeId input,
                     std::vector<Int>&& values, VarNodeId r);

  explicit SetInNode(IInvariantGraph& graph, VarNodeId input,
                     std::vector<Int>&& values, bool shouldHold = true);

  void init(InvariantNodeId) override;

  void registerOutputVars() override;

  void registerNode() override;

  [[nodiscard]] const std::vector<Int>& values() { return _values; }

  virtual std::string dotLangIdentifier() const override;
};
}  // namespace atlantis::invariantgraph
