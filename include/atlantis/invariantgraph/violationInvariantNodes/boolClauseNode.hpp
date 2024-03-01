#pragma once

#include <vector>

#include "atlantis/invariantgraph/invariantGraph.hpp"
#include "atlantis/invariantgraph/types.hpp"
#include "atlantis/invariantgraph/violationInvariantNode.hpp"
#include "atlantis/propagation/solverBase.hpp"
#include "atlantis/propagation/types.hpp"

namespace atlantis::invariantgraph {
class BoolClauseNode : public ViolationInvariantNode {
 private:
  std::vector<VarNodeId> _as;
  std::vector<VarNodeId> _bs;
  propagation::VarId _sumVarId{propagation::NULL_ID};

 public:
  explicit BoolClauseNode(std::vector<VarNodeId>&& as,
                          std::vector<VarNodeId>&& bs, VarNodeId r);

  explicit BoolClauseNode(std::vector<VarNodeId>&& as,
                          std::vector<VarNodeId>&& bs, bool shouldHold);

  void registerOutputVars(InvariantGraph&,
                          propagation::SolverBase& solver) override;

  void registerNode(InvariantGraph&, propagation::SolverBase& solver) override;

  [[nodiscard]] const std::vector<VarNodeId>& as() { return _as; }

  [[nodiscard]] const std::vector<VarNodeId>& bs() { return _bs; }
};
}  // namespace atlantis::invariantgraph
