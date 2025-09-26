#pragma once

#include <atlantis/sortedUniqueVector.hpp>

#include "atlantis/invariantgraph/violationInvariantNode.hpp"

namespace atlantis::invariantgraph {
class SetInNode : public ViolationInvariantNode {
  SortedUniqueVector _values;

 public:
  explicit SetInNode(InvariantGraph& graph, VarNodeId input,
                     std::vector<Int>&& values, VarNodeId r);

  explicit SetInNode(InvariantGraph& graph, VarNodeId input,
                     std::vector<Int>&& values, bool shouldHold = true);

  void init(InvariantNodeId) override;

  void updateState() override;

  void registerOutputVars(propagation::SolverBase&,
                          SolverMapping&) const override;

  void registerNode(propagation::SolverBase&, SolverMapping&) const override;

  [[nodiscard]] const std::vector<Int>& values() const { return *_values; }

  [[nodiscard]] std::string dotLangIdentifier() const override;
};
}  // namespace atlantis::invariantgraph
