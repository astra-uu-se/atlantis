#pragma once

#include "atlantis/invariantgraph/violationInvariantNode.hpp"

namespace atlantis::invariantgraph {

class BoolOrNode : public ViolationInvariantNode {
 public:
  BoolOrNode(InvariantGraph& graph, VarNode& a, VarNode& b, VarNode& r);

  BoolOrNode(InvariantGraph& graph, VarNode& a, VarNode& b,
             bool shouldHold = true);

  void init() override;

  void postConstraint() override;

  void updateState() override;

  [[nodiscard]] bool canBeReplaced() const override;

  bool replace() override;

  void registerOutputVars(propagation::SolverBase&,
                          SolverMapping&) const override;

  void registerNode(propagation::SolverBase&, SolverMapping&) const override;

  [[nodiscard]] VarNode& a() const noexcept {
    return staticInputVarNode(0);
  }
  [[nodiscard]] VarNode& b() const noexcept {
    return *staticInputVarNodes().back();
  }

  [[nodiscard]] std::string dotLangIdentifier() const override;
};

}  // namespace atlantis::invariantgraph
