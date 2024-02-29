#pragma once

#include <map>
#include <utility>

#include "atlantis/invariantgraph/invariantGraph.hpp"
#include "atlantis/invariantgraph/invariantNode.hpp"
#include "atlantis/propagation/views/bool2IntView.hpp"

namespace atlantis::invariantgraph {

class Bool2IntNode : public InvariantNode {
 public:
  Bool2IntNode(VarNodeId staticInput, VarNodeId output);

  ~Bool2IntNode() override = default;

  void registerOutputVars(InvariantGraph&,
                          propagation::SolverBase& solver) override;

  void registerNode(InvariantGraph&, propagation::SolverBase& solver) override;

  [[nodiscard]] VarNodeId input() const noexcept {
    return staticInputVarNodeIds().front();
  }
};

}  // namespace atlantis::invariantgraph
