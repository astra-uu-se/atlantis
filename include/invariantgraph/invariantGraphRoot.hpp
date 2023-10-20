#pragma once

#include "invariantgraph/implicitConstraintNode.hpp"

namespace invariantgraph {

/**
 * Given all variable nodes need to be defined by a a InvariantNode, this
 * class "defines" the search variables so their nodes can be created and
 * registered with the engine.
 */
class InvariantGraphRoot : public ImplicitConstraintNode {
 public:
  InvariantGraphRoot(std::vector<VarNodeId>&& variables = {})
      : ImplicitConstraintNode(std::move(variables)) {}

  ~InvariantGraphRoot() override = default;

  void addSearchVarNode(VarNode&);

 protected:
  std::shared_ptr<search::neighbourhoods::Neighbourhood> createNeighbourhood(
      Engine& engine, std::vector<search::SearchVariable>&& variables) override;
};

}  // namespace invariantgraph