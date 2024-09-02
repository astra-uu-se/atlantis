#pragma once

#include <vector>

#include "atlantis/invariantgraph/invariantGraph.hpp"
#include "atlantis/invariantgraph/types.hpp"
#include "atlantis/invariantgraph/violationInvariantNode.hpp"
#include "atlantis/propagation/solverBase.hpp"
#include "atlantis/propagation/types.hpp"

namespace atlantis::invariantgraph {

class IntAllEqualNode : public ViolationInvariantNode {
 private:
  bool _breaksCycle{false};
  propagation::VarId _allDifferentViolationVarId{propagation::NULL_ID};

 public:
  explicit IntAllEqualNode(VarNodeId a, VarNodeId b, VarNodeId r,
                           bool breaksCycle = false);

  explicit IntAllEqualNode(VarNodeId a, VarNodeId b, bool shouldHold = true,
                           bool breaksCycle = false);

  explicit IntAllEqualNode(std::vector<VarNodeId>&& vars, VarNodeId r,
                           bool breaksCycle = false);

  explicit IntAllEqualNode(std::vector<VarNodeId>&& vars,
                           bool shouldHold = true, bool breaksCycle = false);

  void init(InvariantGraph&, const InvariantNodeId&) override;

  void updateState(InvariantGraph&) override;

  void registerOutputVars(InvariantGraph&, propagation::SolverBase&) override;

  void registerNode(InvariantGraph&, propagation::SolverBase&) override;

  virtual std::string dotLangIdentifier() const override;
};
}  // namespace atlantis::invariantgraph
