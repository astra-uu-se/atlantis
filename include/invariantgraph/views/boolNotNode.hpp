#pragma once

#include <map>
#include <utility>

#include "invariantgraph/invariantGraph.hpp"
#include "invariantgraph/invariantNode.hpp"
#include "propagation/views/bool2IntView.hpp"

namespace atlantis::invariantgraph {

class BoolNotNode : public InvariantNode {
 public:
  BoolNotNode(VarNodeId staticInput, VarNodeId output);

  ~BoolNotNode() override = default;

  void registerOutputVars(InvariantGraph&,
                          propagation::SolverBase& solver) override;

  void registerNode(InvariantGraph&, propagation::SolverBase& solver) override;

  [[nodiscard]] VarNodeId input() const noexcept {
    return staticInputVarNodeIds().front();
  }
};

}  // namespace atlantis::invariantgraph