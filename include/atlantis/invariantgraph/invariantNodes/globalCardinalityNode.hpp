#pragma once

#include <vector>

#include "atlantis/invariantgraph/invariantGraph.hpp"
#include "atlantis/invariantgraph/invariantNode.hpp"
#include "atlantis/invariantgraph/types.hpp"
#include "atlantis/propagation/solverBase.hpp"
#include "atlantis/types.hpp"

namespace atlantis::invariantgraph {
class GlobalCardinalityNode : public InvariantNode {
 private:
  std::vector<Int> _cover;
  std::vector<Int> _countOffsets;
  std::vector<propagation::VarId> _intermediate;

 public:
  explicit GlobalCardinalityNode(std::vector<VarNodeId>&& inputs,
                                 std::vector<Int>&& cover,
                                 std::vector<VarNodeId>&& counts);

  void init(InvariantGraph&, const InvariantNodeId&) override;

  void registerOutputVars(InvariantGraph&, propagation::SolverBase&) override;

  void registerNode(InvariantGraph&, propagation::SolverBase&) override;

  void updateState(InvariantGraph&) override;

  [[nodiscard]] bool canBeReplaced(const InvariantGraph&) const override;

  [[nodiscard]] bool replace(InvariantGraph&) override;
};

}  // namespace atlantis::invariantgraph
