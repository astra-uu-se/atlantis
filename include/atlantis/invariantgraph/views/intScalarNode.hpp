#pragma once

#include "atlantis/invariantgraph/invariantGraph.hpp"
#include "atlantis/invariantgraph/invariantNode.hpp"
#include "atlantis/invariantgraph/types.hpp"
#include "atlantis/propagation/solverBase.hpp"

namespace atlantis::invariantgraph {

class IntScalarNode : public InvariantNode {
 private:
  Int _factor;
  Int _offset;

 public:
  IntScalarNode(VarNodeId staticInput, VarNodeId output, Int factor,
                Int offset);

  void init(InvariantGraph&, const InvariantNodeId&) override;

  void updateState(InvariantGraph&) override;

  void registerOutputVars(InvariantGraph&, propagation::SolverBase&) override;

  void registerNode(InvariantGraph&, propagation::SolverBase&) override;

  [[nodiscard]] VarNodeId input() const noexcept {
    return staticInputVarNodeIds().front();
  }

  std::ostream& dotLangEntry(std::ostream&) const override;

  std::ostream& dotLangEdges(std::ostream&) const override;

  std::string dotLangIdentifier() const override;
};

}  // namespace atlantis::invariantgraph
