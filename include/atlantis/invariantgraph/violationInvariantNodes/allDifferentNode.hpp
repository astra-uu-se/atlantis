#pragma once

#include <vector>

#include "atlantis/invariantgraph/invariantGraph.hpp"
#include "atlantis/invariantgraph/types.hpp"
#include "atlantis/invariantgraph/violationInvariantNode.hpp"
#include "atlantis/propagation/solverBase.hpp"
#include "atlantis/propagation/types.hpp"

namespace atlantis::invariantgraph {
class AllDifferentNode : public ViolationInvariantNode {
 private:
  propagation::VarId _intermediate{propagation::NULL_ID};

 public:
  explicit AllDifferentNode(VarNodeId a, VarNodeId b, VarNodeId r);

  explicit AllDifferentNode(VarNodeId a, VarNodeId b, bool shouldHold = true);

  explicit AllDifferentNode(std::vector<VarNodeId>&& vars, VarNodeId r);

  explicit AllDifferentNode(std::vector<VarNodeId>&& vars,
                            bool shouldHold = true);

  void init(InvariantGraph&, const InvariantNodeId&) override;

  virtual void updateState(InvariantGraph&) override;

  [[nodiscard]] bool canBeMadeImplicit(const InvariantGraph&) const override;

  [[nodiscard]] bool makeImplicit(InvariantGraph&) override;

  void registerOutputVars(InvariantGraph&, propagation::SolverBase&) override;

  void registerNode(InvariantGraph&, propagation::SolverBase&) override;

  virtual std::string dotLangIdentifier() const override;
};
}  // namespace atlantis::invariantgraph
