#pragma once

#include <atlantis/sortedUniqueVector.hpp>

#include "atlantis/invariantgraph/violationInvariantNode.hpp"

namespace atlantis::invariantgraph {
class SetInNode : public ViolationInvariantNode {
  SortedUniqueVector _values;

 public:
  explicit SetInNode(InvariantGraph& graph, VarNode& input,
                     std::vector<Int>&& values, VarNode& r);

  explicit SetInNode(InvariantGraph& graph, VarNode& input,
                     std::vector<Int>&& values, bool shouldHold = true);

  void init() override;

  void postConstraint() override;

  void updateState() override;

  void registerOutputVars(propagation::SolverBase&,
                          SolverMapping&) const override;

  void registerNode(propagation::SolverBase&, SolverMapping&) const override;

  [[nodiscard]] const std::vector<Int>& values() const {
    return *_values.operator->();
  }

  [[nodiscard]] std::string dotLangIdentifier() const override;
};
}  // namespace atlantis::invariantgraph
