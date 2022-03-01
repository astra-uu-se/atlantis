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
  std::vector<std::unique_ptr<InvariantNode>> _invariants;
  std::vector<std::unique_ptr<SoftConstraintNode>> _softConstraints;
  std::vector<std::unique_ptr<ImplicitConstraintNode>> _implicitConstraints;

 public:
  std::unique_ptr<invariantgraph::InvariantGraph> build(
      const std::unique_ptr<fznparser::Model>& model);

 private:
  void createNodes(const std::unique_ptr<fznparser::Model>& model);

  bool canBeImplicit(const ConstraintRef& constraint);
  bool allVariablesFree(const ConstraintRef& constraint,
                        const std::unordered_set<VarRef>& definedVars);

  std::unique_ptr<InvariantNode> makeInvariant(const ConstraintRef& constraint);
  std::unique_ptr<ImplicitConstraintNode> makeImplicitConstraint(
      const ConstraintRef& constraint);
  std::unique_ptr<SoftConstraintNode> makeSoftConstraint(
      const ConstraintRef& constraint);
  std::unique_ptr<ViewNode> makeView(const ConstraintRef& constraint);
};
}  // namespace invariantgraph