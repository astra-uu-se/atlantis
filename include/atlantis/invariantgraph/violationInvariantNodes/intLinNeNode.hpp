#pragma once

#include <vector>

#include "atlantis/invariantgraph/invariantGraph.hpp"
#include "atlantis/invariantgraph/types.hpp"
#include "atlantis/invariantgraph/violationInvariantNode.hpp"
#include "atlantis/propagation/solverBase.hpp"
#include "atlantis/propagation/types.hpp"

namespace atlantis::invariantgraph {

class IntLinNeNode : public ViolationInvariantNode {
 private:
  std::vector<Int> _coeffs;
  Int _bound;
  propagation::VarId _intermediate{propagation::NULL_ID};

 public:
  IntLinNeNode(InvariantGraph& graph,

               std::vector<Int>&& coeffs, std::vector<VarNodeId>&& vars,
               Int bound, bool shouldHold = true);

  IntLinNeNode(InvariantGraph& graph,

               std::vector<Int>&& coeffs, std::vector<VarNodeId>&& vars,
               Int bound, VarNodeId reified);

  void init(const InvariantNodeId&) override;

  void updateState() override;

  void registerOutputVars() override;

  void registerNode() override;

  [[nodiscard]] const std::vector<Int>& coeffs() const;
};

}  // namespace atlantis::invariantgraph
