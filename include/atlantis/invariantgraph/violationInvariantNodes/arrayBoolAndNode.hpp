#pragma once

#include "atlantis/invariantgraph/violationInvariantNode.hpp"

namespace atlantis::invariantgraph {

class ArrayBoolAndNode : public ViolationInvariantNode {
 public:
  ArrayBoolAndNode(InvariantGraph& graph, VarNode& a, VarNode& b,
                   VarNode& output);

  ArrayBoolAndNode(InvariantGraph& graph, VarNode& a, VarNode& b,
                   bool shouldHold = true);

  ArrayBoolAndNode(InvariantGraph& graph,
                   std::vector<std::shared_ptr<VarNode>>&& as, VarNode& output);

  ArrayBoolAndNode(InvariantGraph& graph,
                   std::vector<std::shared_ptr<VarNode>>&& as,
                   bool shouldHold = true);

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
