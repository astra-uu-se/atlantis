#pragma once

#include <vector>

#include "atlantis/invariantgraph/implicitConstraintNode.hpp"
#include "atlantis/invariantgraph/types.hpp"
#include "atlantis/propagation/solverBase.hpp"

namespace atlantis::invariantgraph {

/**
 * Given all variable nodes need to be defined by a a InvariantNode, this
 * class "defines" the search variables so their nodes can be created and
 * registered with the solver.
 */
class InvariantGraphRoot : public ImplicitConstraintNode {
 public:
  explicit InvariantGraphRoot(std::vector<VarNodeId>&& vars = {});

  void addSearchVarNode(VarNode&);

 protected:
  std::shared_ptr<search::neighbourhoods::Neighbourhood> createNeighbourhood()
      override;
};

}  // namespace atlantis::invariantgraph
