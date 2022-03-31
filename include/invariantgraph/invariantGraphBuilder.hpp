#pragma once

#include <memory>
#include <optional>
#include <unordered_map>
#include <unordered_set>
#include <variant>

#include "fznparser/model.hpp"
#include "invariantGraph.hpp"

namespace invariantgraph {
class InvariantGraphBuilder {
 private:
  typedef std::shared_ptr<fznparser::Variable> VarRef;
  typedef std::shared_ptr<fznparser::Constraint> ConstraintRef;

  std::unordered_map<VarRef, VariableNode*> _variableMap;
  std::vector<std::unique_ptr<VariableNode>> _variables;
  std::vector<std::unique_ptr<VariableDefiningNode>> _definingNodes;

 public:
  std::unique_ptr<invariantgraph::InvariantGraph> build(
      const std::unique_ptr<fznparser::Model>& model);

 private:
  void createNodes(const std::unique_ptr<fznparser::Model>& model);

  bool allVariablesFree(const ConstraintRef& constraint,
                        const std::unordered_set<VarRef>& definedVars);

  std::unique_ptr<VariableDefiningNode> makeVariableDefiningNode(
      const ConstraintRef& constraint);
  std::unique_ptr<ImplicitConstraintNode> makeImplicitConstraint(
      const ConstraintRef& constraint);
  std::unique_ptr<SoftConstraintNode> makeSoftConstraint(
      const ConstraintRef& constraint);
};
}  // namespace invariantgraph