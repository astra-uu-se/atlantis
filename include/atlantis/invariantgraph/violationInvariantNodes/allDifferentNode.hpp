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
  explicit AllDifferentNode(InvariantGraph& graph,

                            VarNodeId a, VarNodeId b, VarNodeId r);

  explicit AllDifferentNode(InvariantGraph& graph,

                            VarNodeId a, VarNodeId b, bool shouldHold = true);

  explicit AllDifferentNode(InvariantGraph& graph,

                            std::vector<VarNodeId>&& vars, VarNodeId r);

  explicit AllDifferentNode(InvariantGraph& graph,

                            std::vector<VarNodeId>&& vars,
                            bool shouldHold = true);

  void init(const InvariantNodeId&) override;

  virtual void updateState() override;

  [[nodiscard]] bool canBeMadeImplicit() const override;

  [[nodiscard]] bool makeImplicit() override;

  void registerOutputVars() override;

  void registerNode() override;
};
}  // namespace atlantis::invariantgraph
