#pragma once

#include <vector>

#include "atlantis/invariantgraph/invariantGraph.hpp"
#include "atlantis/invariantgraph/invariantNode.hpp"
#include "atlantis/invariantgraph/types.hpp"
#include "atlantis/propagation/solverBase.hpp"
#include "atlantis/types.hpp"

namespace atlantis::invariantgraph {
class BoolLinearNode : public InvariantNode {
 private:
  std::vector<Int> _coeffs;
  Int _offset{0};
  propagation::VarId _intermediate{propagation::NULL_ID};

 public:
  BoolLinearNode(InvariantGraph& graph,

                 std::vector<Int>&& coeffs, std::vector<VarNodeId>&& vars,
                 VarNodeId output, Int offset = 0);

  void init(const InvariantNodeId&) override;

  void updateState() override;

  void registerOutputVars() override;

  void registerNode() override;

  [[nodiscard]] const std::vector<Int>& coeffs() const;
};
}  // namespace atlantis::invariantgraph
