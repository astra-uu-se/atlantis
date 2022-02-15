#include "invariantgraph/invariantGraphBuilder.hpp"

#include <algorithm>

#include "invariantgraph/constraints/allDifferent.hpp"
#include "invariantgraph/invariants/linear.hpp"
#include "invariantgraph/invariants/max.hpp"

std::unique_ptr<invariantgraph::InvariantGraph>
invariantgraph::InvariantGraphBuilder::build(
    const std::unique_ptr<fznparser::Model>& model) {
  _variableNodes.clear();

  createNodes(model);

  std::vector<std::shared_ptr<invariantgraph::VariableNode>> variables;
  std::transform(_variableNodes.begin(), _variableNodes.end(),
                 std::back_inserter(variables),
                 [](auto pair) { return pair.second; });

  auto graph = std::make_unique<invariantgraph::InvariantGraph>(variables);
  return graph;
}

void invariantgraph::InvariantGraphBuilder::createNodes(
    const std::unique_ptr<fznparser::Model>& model) {
  for (const auto& variable : model->variables()) {
    if (variable->type() == fznparser::LiteralType::VARIABLE_ARRAY) {
      continue;
    }

    _variableNodes.emplace(
        variable,
        std::make_shared<VariableNode>(
            std::dynamic_pointer_cast<fznparser::SearchVariable>(variable)));
  }

  std::unordered_set<VarRef> definedVars;
  std::unordered_set<ConstraintRef> processedConstraints;

  // First, define based on annotations.
  for (const auto& constraint : model->constraints()) {
    if (!constraint->annotations().has<fznparser::DefinesVarAnnotation>()) {
      continue;
    }

    auto ann = constraint->annotations().get<fznparser::DefinesVarAnnotation>();
    if (definedVars.count(ann->defines().lock())) {
      continue;
    }

    auto invariant = makeInvariant(constraint);

    definedVars.emplace(invariant->output()->variable());
    processedConstraints.emplace(constraint);
  }

  // Second, define an implicit constraint (neighborhood) on remaining
  // constraints.
  for (const auto& constraint : model->constraints()) {
    if (processedConstraints.count(constraint) || !canBeImplicit(constraint) ||
        !allVariablesFree(constraint, definedVars)) {
      continue;
    }

    auto implicitConstraint = makeImplicitConstraint(constraint);
    _implicitConstraints.emplace(constraint, implicitConstraint);
    for (const auto& variableNode : implicitConstraint->definingVariables()) {
      definedVars.emplace(variableNode->variable());
    }
    processedConstraints.emplace(constraint);
  }

  // Third, define soft constraints.
  for (const auto& constraint : model->constraints()) {
    if (processedConstraints.count(constraint)) {
      continue;
    }

    makeSoftConstraint(constraint);
  }
}

bool invariantgraph::InvariantGraphBuilder::canBeImplicit(
    const ConstraintRef& constraint) {
  // TODO: Implicit constraints
  return false;
}

// ==========================================================================
// See the example at https://en.cppreference.com/w/cpp/utility/variant/visit
template <class... Ts>
struct overloaded : Ts... {
  using Ts::operator()...;
};
// explicit deduction guide (not needed as of C++20)
template <class... Ts>
overloaded(Ts...) -> overloaded<Ts...>;
// ==========================================================================

bool invariantgraph::InvariantGraphBuilder::allVariablesFree(
    const ConstraintRef& constraint,
    const std::unordered_set<VarRef>& definedVars) {
  auto isFree =
      [&definedVars](const std::shared_ptr<fznparser::Literal>& literal) {
        if (literal->type() != fznparser::LiteralType::SEARCH_VARIABLE)
          return false;

        return definedVars.count(
                   std::dynamic_pointer_cast<fznparser::Variable>(literal)) > 0;
      };

  for (const auto& arg : constraint->arguments()) {
    bool free = true;
    std::visit(
        overloaded{[&free, &isFree](
                       const std::shared_ptr<fznparser::Literal>& literal) {
                     if (!isFree(literal)) free = false;
                   },
                   [&free, &isFree](
                       const std::vector<std::shared_ptr<fznparser::Literal>>&
                           literals) {
                     if (!std::all_of(literals.begin(), literals.end(), isFree))
                       free = false;
                   }},
        arg);

    if (!free) return false;
  }

  return true;
}

typedef std::shared_ptr<fznparser::Variable> VarRef;
typedef std::shared_ptr<fznparser::Constraint> ConstraintRef;
typedef std::unordered_map<VarRef,
                           std::shared_ptr<invariantgraph::VariableNode>>
    VarMap;

std::shared_ptr<invariantgraph::InvariantNode>
invariantgraph::InvariantGraphBuilder::makeInvariant(
    const ConstraintRef& constraint) {
  std::string_view name = constraint->name();
  if (name == "int_lin_eq") {
    return std::static_pointer_cast<invariantgraph::InvariantNode>(
        invariantgraph::LinearInvariantNode::fromModelConstraint(
            constraint, [this](auto var) { return _variableNodes.at(var); }));
  } else if (name == "int_max" || name == "array_int_maximum") {
    return std::static_pointer_cast<invariantgraph::InvariantNode>(
        invariantgraph::MaxInvariantNode::fromModelConstraint(
            constraint, [this](auto var) { return _variableNodes.at(var); }));
  }

  throw std::runtime_error("Unsupported constraint: " + std::string(name));
}

invariantgraph::InvariantGraphBuilder::ImplicitConstraintRef
invariantgraph::InvariantGraphBuilder::makeImplicitConstraint(
    const ConstraintRef& constraint) {
  return std::make_shared<invariantgraph::ImplicitConstraintNode>();
}

invariantgraph::InvariantGraphBuilder::SoftConstraintRef
invariantgraph::InvariantGraphBuilder::makeSoftConstraint(
    const ConstraintRef& constraint) {
  std::string_view name = constraint->name();
  if (name == "alldifferent") {
    return invariantgraph::AllDifferentNode::fromModelConstraint(
        constraint, [this](auto var) { return _variableNodes.at(var); });
  }

  throw std::runtime_error(std::string("Failed to create soft constraint: ")
                               .append(constraint->name()));
}