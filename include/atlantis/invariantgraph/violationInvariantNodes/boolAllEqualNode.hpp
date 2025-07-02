#pragma once

#include "atlantis/invariantgraph/violationInvariantNode.hpp"

namespace atlantis::invariantgraph {
class BoolAllEqualNode : public ViolationInvariantNode {
  bool _breaksCycle{false};
  unsigned char _dom{2};
  propagation::VarViewId _intermediate{propagation::NULL_ID};

  [[nodiscard]] bool isFixed() const;
  [[nodiscard]] bool inDomain(bool) const;
  [[nodiscard]] bool holdsTrue() const;
  [[nodiscard]] bool holdsFalse() const;
  void fixToVal(bool);

 public:
  explicit BoolAllEqualNode(InvariantGraph& graph, VarNodeId a, VarNodeId b,
                            VarNodeId r, bool breaksCycle = false);

  explicit BoolAllEqualNode(InvariantGraph& graph, VarNodeId a, VarNodeId b,
                            bool shouldHold = true, bool breaksCycle = false);

  explicit BoolAllEqualNode(InvariantGraph& graph,
                            std::vector<VarNodeId>&& vars, VarNodeId r,
                            bool breaksCycle = false);

  explicit BoolAllEqualNode(InvariantGraph& graph,
                            std::vector<VarNodeId>&& vars,
                            bool shouldHold = true, bool breaksCycle = false);

  void init(InvariantNodeId) override;

  void updateState() override;

  [[nodiscard]] bool canBeReplaced() const override;

  bool replace() override;

  void registerOutputVars() override;

  void registerNode() override;
  [[nodiscard]] std::string dotLangIdentifier() const override;
};
}  // namespace atlantis::invariantgraph
