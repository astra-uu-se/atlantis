#pragma once

#include <vector>

#include "atlantis/invariantgraph/invariantGraph.hpp"
#include "atlantis/invariantgraph/types.hpp"
#include "atlantis/invariantgraph/violationInvariantNode.hpp"
#include "atlantis/propagation/solverBase.hpp"
#include "atlantis/propagation/types.hpp"

namespace atlantis::invariantgraph {
class SetInNode : public ViolationInvariantNode {
 private:
  std::vector<Int> _values;
  propagation::VarId _intermediate{propagation::NULL_ID};

 public:
  explicit SetInNode(InvariantGraph& graph,

                     VarNodeId input, std::vector<Int>&& values, VarNodeId r);

  explicit SetInNode(InvariantGraph& graph,

                     VarNodeId input, std::vector<Int>&& values,
                     bool shouldHold = true);

  void init(const InvariantNodeId&) override;

  void registerOutputVars() override;

  void registerNode() override;

  [[nodiscard]] const std::vector<Int>& values() { return _values; }
};
}  // namespace atlantis::invariantgraph
