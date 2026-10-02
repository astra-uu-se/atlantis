#pragma once

#include "atlantis/invariantgraph/violationInvariantNode.hpp"

namespace atlantis::invariantgraph {

class ArrayBoolOrNode : public ViolationInvariantNode {
 public:
  ArrayBoolOrNode(InvariantGraph& graph, VarNode& a, VarNode& b,
                  VarNode& reified);

  ArrayBoolOrNode(InvariantGraph& graph, VarNode& a, VarNode& b,
                  bool shouldHold = true);

  ArrayBoolOrNode(InvariantGraph& graph,
                  std::vector<std::shared_ptr<VarNode>>&& inputs,
                  VarNode& reified);

  ArrayBoolOrNode(InvariantGraph& graph,
                  std::vector<std::shared_ptr<VarNode>>&& inputs,
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
