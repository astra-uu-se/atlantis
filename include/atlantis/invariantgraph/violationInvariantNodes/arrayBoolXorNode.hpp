#pragma once

#include "atlantis/invariantgraph/violationInvariantNode.hpp"

namespace atlantis::invariantgraph {

class ArrayBoolXorNode : public ViolationInvariantNode {
  std::optional<bool> _containsFixedTrue{std::nullopt};

 public:
  ArrayBoolXorNode(InvariantGraph& graph, VarNode& a, VarNode& b,
                   VarNode& reified);

  ArrayBoolXorNode(InvariantGraph& graph, VarNode& a, VarNode& b,
                   bool shouldHold = true);

  ArrayBoolXorNode(InvariantGraph& graph,
                   std::vector<std::shared_ptr<VarNode>>&& inputs,
                   VarNode& reified);

  ArrayBoolXorNode(InvariantGraph& graph,
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
