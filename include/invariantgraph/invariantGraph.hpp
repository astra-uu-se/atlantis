#pragma once

#include <map>
#include <memory>
#include <set>
#include <vector>

#include "core/engine.hpp"
#include "structure.hpp"

namespace invariantgraph {

class InvariantGraph {
 private:
  std::vector<std::unique_ptr<VariableNode>> _variables;
  std::vector<std::unique_ptr<VariableDefiningNode>> _variableDefiningNodes;
  std::vector<ImplicitConstraintNode*> _implicitConstraints;

  friend class InvariantGraphBuilder;

 public:
  InvariantGraph(
      std::vector<std::unique_ptr<VariableNode>> variables,
      std::vector<std::unique_ptr<VariableDefiningNode>> variableDefiningNodes)
      : _variables(std::move(variables)),
        _variableDefiningNodes(std::move(variableDefiningNodes)) {
    for (const auto& definingNode : _variableDefiningNodes) {
      if (auto implicitConstraint =
              dynamic_cast<ImplicitConstraintNode*>(definingNode.get())) {
        _implicitConstraints.push_back(implicitConstraint);
      }
    }
  }

  void apply(Engine& engine);
};

}  // namespace invariantgraph