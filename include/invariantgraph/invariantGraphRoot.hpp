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
  explicit InvariantGraphRoot(std::vector<VariableNode*> variables)
      : ImplicitConstraintNode(std::move(variables)) {}

  ~InvariantGraphRoot() override = default;

  void addSearchVariable(VariableNode* node);

 protected:
  search::neighbourhoods::Neighbourhood* createNeighbourhood(
      Engine& engine, std::vector<search::SearchVariable> variables) override;
};

}  // namespace invariantgraph