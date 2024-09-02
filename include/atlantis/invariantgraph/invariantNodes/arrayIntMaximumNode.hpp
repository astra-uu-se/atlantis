#pragma once

#include <vector>

#include "atlantis/invariantgraph/invariantGraph.hpp"
#include "atlantis/invariantgraph/invariantNode.hpp"
#include "atlantis/invariantgraph/types.hpp"
#include "atlantis/propagation/solverBase.hpp"

namespace atlantis::invariantgraph {
class ArrayIntMaximumNode : public InvariantNode {
 private:
  Int _lb;

 public:
  explicit ArrayIntMaximumNode(VarNodeId a, VarNodeId b, VarNodeId output);

  explicit ArrayIntMaximumNode(std::vector<VarNodeId>&& vars, VarNodeId output);

  void init(InvariantGraph&, const InvariantNodeId&) override;

  void registerOutputVars(InvariantGraph&, propagation::SolverBase&) override;

  void updateState(InvariantGraph&) override;

  [[nodiscard]] bool canBeReplaced(const InvariantGraph&) const override;

  [[nodiscard]] bool replace(InvariantGraph&) override;

  void registerNode(InvariantGraph&, propagation::SolverBase&) override;

  virtual std::string dotLangIdentifier() const override;
};
}  // namespace atlantis::invariantgraph
