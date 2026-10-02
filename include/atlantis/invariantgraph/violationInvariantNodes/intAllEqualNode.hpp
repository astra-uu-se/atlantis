#pragma once

#include <optional>

#include "atlantis/invariantgraph/violationInvariantNode.hpp"

namespace atlantis::invariantgraph {

class IntAllEqualNode : public ViolationInvariantNode {
  std::optional<Int> _boundVal{std::nullopt};
  bool _breaksCycle;

 public:
  explicit IntAllEqualNode(InvariantGraph& graph, VarNode& a, VarNode& b,
                           VarNode& r, bool breaksCycle = false);

  explicit IntAllEqualNode(InvariantGraph& graph, VarNode& a, VarNode& b,
                           bool shouldHold = true, bool breaksCycle = false);

  explicit IntAllEqualNode(InvariantGraph& graph, std::vector<std::shared_ptr<VarNode>>&& vars,
                           VarNode& r, bool breaksCycle = false);

  explicit IntAllEqualNode(InvariantGraph& graph, std::vector<std::shared_ptr<VarNode>>&& vars,
                           bool shouldHold = true, bool breaksCycle = false);

  void init() override;

  void postConstraint() override;

  void updateState() override;

  [[nodiscard]] bool canBeReplaced() const override;

  bool replace() override;

  void registerOutputVars(propagation::SolverBase&,
                          SolverMapping&) const override;

  void registerNode(propagation::SolverBase&, SolverMapping&) const override;
  [[nodiscard]] std::string dotLangIdentifier() const override;
};
}  // namespace atlantis::invariantgraph
