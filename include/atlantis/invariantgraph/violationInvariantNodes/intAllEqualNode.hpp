#pragma once

#include <optional>

#include "atlantis/invariantgraph/violationInvariantNode.hpp"

namespace atlantis::invariantgraph {

class IntAllEqualNode : public ViolationInvariantNode {
  bool _breaksCycle{false};

  std::optional<Int> _boundVal;

 public:
  explicit IntAllEqualNode(InvariantGraph& graph, VarNodeId a, VarNodeId b,
                           VarNodeId r, bool breaksCycle = false);

  explicit IntAllEqualNode(InvariantGraph& graph, VarNodeId a, VarNodeId b,
                           bool shouldHold = true, bool breaksCycle = false);

  explicit IntAllEqualNode(InvariantGraph& graph, std::vector<VarNodeId>&& vars,
                           VarNodeId r, bool breaksCycle = false);

  explicit IntAllEqualNode(InvariantGraph& graph, std::vector<VarNodeId>&& vars,
                           bool shouldHold = true, bool breaksCycle = false);

  void init(InvariantNodeId) override;

  void updateState() override;

  [[nodiscard]] bool canBeReplaced() const override;

  bool replace() override;

  void registerOutputVars(propagation::SolverBase&,
                          SolverMapping&) const override;

  void registerNode(propagation::SolverBase&, SolverMapping&) const override;
  [[nodiscard]] std::string dotLangIdentifier() const override;
};
}  // namespace atlantis::invariantgraph
