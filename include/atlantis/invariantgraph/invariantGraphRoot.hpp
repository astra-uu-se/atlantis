#pragma once

#include <vector>

#include "atlantis/invariantgraph/implicitConstraintNode.hpp"
#include "atlantis/invariantgraph/types.hpp"

namespace atlantis::invariantgraph {

/**
 * Given all variable nodes need to be defined by a a InvariantNode, this
 * class "defines" the search variables so their nodes can be created and
 * registered with the solver.
 */
class InvariantGraphRoot : public ImplicitConstraintNode {
 public:
  explicit InvariantGraphRoot(InvariantGraph& graph,
                              std::vector<VarNodeId>&& vars = {});

  void addSearchVarNode(VarNodeId);

  [[nodiscard]] std::ostream& dotLangEntry(std::ostream&) const override;

  [[nodiscard]] std::ostream& dotLangEdges(std::ostream&) const override;

  void updateDomainTypes() override;

  void registerNode(propagation::SolverBase&, SolverMapping&) const override;

protected:
  [[nodiscard]] std::string dotLangIdentifier() const override;
};

}  // namespace atlantis::invariantgraph
