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

  typedef std::shared_ptr<InvariantNode> InvariantRef;
  typedef std::shared_ptr<ImplicitConstraintNode> ImplicitConstraintRef;
  typedef std::shared_ptr<SoftConstraintNode> SoftConstraintRef;

  std::unordered_map<VarRef, std::shared_ptr<VariableNode>> _variableNodes;

  std::unordered_map<ConstraintRef, ImplicitConstraintRef> _implicitConstraints;
  std::unordered_map<ConstraintRef, SoftConstraintRef> _softConstraints;

 public:
  std::unique_ptr<invariantgraph::InvariantGraph> build(
      const std::unique_ptr<fznparser::Model>& model);

 private:
  void createNodes(const std::unique_ptr<fznparser::Model>& model);

  bool canBeImplicit(const ConstraintRef& constraint);
  bool allVariablesFree(const ConstraintRef& constraint,
                        const std::unordered_set<VarRef>& definedVars);

  std::shared_ptr<InvariantNode> makeInvariant(const ConstraintRef& constraint);
  ImplicitConstraintRef makeImplicitConstraint(const ConstraintRef& constraint);
  SoftConstraintRef makeSoftConstraint(const ConstraintRef& constraint);
};
}  // namespace invariantgraph