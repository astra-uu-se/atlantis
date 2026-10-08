#pragma once

#include "atlantis/invariantgraph/invariantNode.hpp"

namespace atlantis::invariantgraph {

class IntScalarNode : public InvariantNode {
  Int _factor;
  Int _offset;

 public:
  IntScalarNode(InvariantGraph& graph, VarNode& staticInput, VarNode& output,
                Int factor, Int offset);

  void init() override;

  void updateState() override;

  [[nodiscard]] bool constrainsOutput(
      const VarNode& outputVarNode) const override;

  [[nodiscard]] bool canBeReplaced() const override;

  bool replace() override;

  void registerOutputVars(propagation::SolverBase&,
                          SolverMapping&) const override;

  void registerNode(propagation::SolverBase&, SolverMapping&) const override;

  [[nodiscard]] VarNode& input() const noexcept {
    return *staticInputVarNodes().front();
  }

  std::ostream& dotLangEntry(std::ostream&) const override;

  std::ostream& dotLangEdges(std::ostream&) const override;

  [[nodiscard]] std::string dotLangIdentifier() const override;
};

}  // namespace atlantis::invariantgraph
