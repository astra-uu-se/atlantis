#include "invariantgraph/invariantGraphBuilder.hpp"

#include <unordered_set>

#include "invariantgraph/constraints/allDifferentNode.hpp"
#include "invariantgraph/constraints/allEqualNode.hpp"
#include "invariantgraph/constraints/arrayBoolAndNode.hpp"
#include "invariantgraph/constraints/arrayBoolOrNode.hpp"
#include "invariantgraph/constraints/boolAndNode.hpp"
#include "invariantgraph/constraints/boolClauseNode.hpp"
#include "invariantgraph/constraints/boolEqNode.hpp"
#include "invariantgraph/constraints/boolLeNode.hpp"
#include "invariantgraph/constraints/boolLinEqNode.hpp"
#include "invariantgraph/constraints/boolLinLeNode.hpp"
#include "invariantgraph/constraints/boolLtNode.hpp"
#include "invariantgraph/constraints/boolOrNode.hpp"
#include "invariantgraph/constraints/boolXorNode.hpp"
#include "invariantgraph/constraints/intEqNode.hpp"
#include "invariantgraph/constraints/intLeNode.hpp"
#include "invariantgraph/constraints/intLinEqNode.hpp"
#include "invariantgraph/constraints/intLinLeNode.hpp"
#include "invariantgraph/constraints/intLinNeNode.hpp"
#include "invariantgraph/constraints/intLtNode.hpp"
#include "invariantgraph/constraints/intNeNode.hpp"
#include "invariantgraph/constraints/setInNode.hpp"
#include "invariantgraph/implicitConstraints/allDifferentImplicitNode.hpp"
#include "invariantgraph/implicitConstraints/circuitImplicitNode.hpp"
#include "invariantgraph/invariantGraphRoot.hpp"
#include "invariantgraph/invariants/arrayBoolElement2dNode.hpp"
#include "invariantgraph/invariants/arrayBoolElementNode.hpp"
#include "invariantgraph/invariants/arrayIntElement2dNode.hpp"
#include "invariantgraph/invariants/arrayIntElementNode.hpp"
#include "invariantgraph/invariants/arrayIntMaximumNode.hpp"
#include "invariantgraph/invariants/arrayIntMinimumNode.hpp"
#include "invariantgraph/invariants/arrayVarBoolElement2dNode.hpp"
#include "invariantgraph/invariants/arrayVarBoolElementNode.hpp"
#include "invariantgraph/invariants/arrayVarIntElement2dNode.hpp"
#include "invariantgraph/invariants/arrayVarIntElementNode.hpp"
#include "invariantgraph/invariants/boolLinearNode.hpp"
#include "invariantgraph/invariants/intDivNode.hpp"
#include "invariantgraph/invariants/intLinearNode.hpp"
#include "invariantgraph/invariants/intMaxNode.hpp"
#include "invariantgraph/invariants/intMinNode.hpp"
#include "invariantgraph/invariants/intModNode.hpp"
#include "invariantgraph/invariants/intPlusNode.hpp"
#include "invariantgraph/invariants/intPowNode.hpp"
#include "invariantgraph/invariants/intTimesNode.hpp"
#include "invariantgraph/views/bool2IntNode.hpp"
#include "invariantgraph/views/boolNotNode.hpp"
#include "invariantgraph/views/intAbsNode.hpp"
#include "utils/fznAst.hpp"

invaraintgraph::InvariantGraphBuilder::InvariantGraphBuilder(
    fznparser::Model&& _model)
    : _model(std::move(_model)), _invariantGraph() {}

invariantgraph::InvariantGraph invariantgraph::InvariantGraphBuilder::build() {
  initVariableNodes(_model);

  createNodes()

      std::optional<Variable>
          objectiveVariable = _model.solveType().objective();
}

using FZNSearchVariable = std::variant<fznparser::IntVar, fznparser::BoolVar>;

static std::optional<FZNSearchVariable> searchVariable(
    const fznparser::Variable& variable) {
  return std::visit<std::optional<FZNSearchVariable>>(
      overloaded{[](const fznparser::IntVar& var) { return var; },
                 [](const fznparser::BoolVar& var) { return var; },
                 [](const auto&) { return std::nullopt; }},
      variable);
}

static bool allVariablesFree(
    const fznparser::Model& _model, const fznparser::Constraint& constraint,
    const std::unordered_set<std::string_view>& definedVars);

static void markVariablesAsDefined(
    const invariantgraph::VariableDefiningNode& node,
    std::unordered_set<std::string_view>& definedVars) {
  for (const auto& variableNode : node.definedVariables()) {
    if (variableNode->variable()) {
      definedVars.emplace(identifier(*variableNode->variable()));
    }
  }
}

void invariantgraph::InvariantGraphBuilder::initVariableNodes(
    const fznparser::Model& _model) {
  for (const auto& variable : _model.variables()) {
    if (std::holds_alternative<fznparser::BolVar>(variable)) {
      _invariantGraph.addNode(std::get<fznparser::BolVar>(variable));
    } else if (std::holds_alternative<fznparser::IntVar>(variable)) {
      _invariantGraph.addNode(std::get<fznparser::IntVar>(variable));
    }
  }
}

void invariantgraph::InvariantGraphBuilder::createNodes() {
  std::unordered_set<std::string_view> definedVars;
  std::vector<bool> constraintIsProcessed(_model.constraints().size(), false);

  // First, define variables based on annotations.
  for (size_t idx = 0; idx < _model.constraints().size(); ++idx) {
    auto& constraint = _model.constraints().at(idx);

    std::optional<std::reference_wrapper<const fznparser::Variable>>
        definedVar = constraint.definedVariable(_model);

    if (!definedVariable.has_value()) {
      continue;
    }

    if (definedVars.count(annotation->definedVariable)) {
      continue;
    }

    std::unique_ptr<VariableDefiningNode> variableDefiningNode =
        makeVariableDefiningNode(constraint);

    if (variableDefiningNode == nullptr) {
      continue;
    }

    variableDefiningNode->prune();
    markVariablesAsDefined(variableDefiningNode.value(), definedVars);

    constraintIsProcessed.at(idx) = true;
  }

  // Second, define implicit constraints (neighborhood)
  for (size_t idx = 0; idx < _model.constraints().size(); ++idx) {
    auto constraint = _model.constraints().at(idx);

    if (constraintIsProcessed.at(idx) || !allVariablesFree(definedVars)) {
      continue;
    }

    if (std::unique_ptr<ImplicitConstraintNode> implicitConstraint =
            makeImplicitConstraint(constraint)) {
      implicitConstraint->prune();

      for (auto variableNode : implicitConstraint->definedVariables()) {
        definedVars.emplace(identifier(*variableNode->variable()));
      }

      _intVariableNodes.push_back(std::move(implicitConstraint));
      constraintIsProcessed.at(idx) = true;
    }
  }

  // Third, pick up any invariants we can identify without the annotations
  for (size_t idx = 0; idx < _model.constraints().size(); ++idx) {
    auto constraint = _model.constraints().at(idx);
    if (constraintIsProcessed.at(idx)) {
      continue;
    }

    if (std::unique_ptr<invariantgraph::VariableDefiningNode>
            variableDefiningNode =
                makeVariableDefiningNode(_model, constraint, true)) {
      auto canBeInvariant = std::all_of(
          variableDefiningNode->definedVariables().begin(),
          variableDefiningNode->definedVariables().end(),
          [&](const auto& variableNode) {
            return !variableNode->variable() ||
                   definedVars.contains(identifier(*variableNode->variable()));
          });

      if (!canBeInvariant) {
        continue;
      }

      variableDefiningNode->prune();

      markVariablesAsDefined(*variableDefiningNode, definedVars);

      _intVariableNodes.push_back(std::move(node));
      constraintIsProcessed.at(idx) = true;
    }
  }

  // Fourth, use soft constraints for the remaining _model constraints.
  for (size_t idx = 0; idx < _model.constraints().size(); ++idx) {
    auto constraint = _model.constraints().at(idx);
    if (constraintIsProcessed(idx)) {
      continue;
    }

    _intVariableNodes.push_back(makeSoftConstraint(constraint));
    constraintIsProcessed.at(idx) = true;
  }

  assert(std::all_of(constraintIsProcessed.begin(), constraintIsProcessed.end(),
                     [](bool b) { return b; }));

  // Finally, define all free variables by the InvariantGraphRoot
  std::vector<std::reference_wrapper<VariableNode>> freeVariables;

  for (const auto& variable : _model.variables()) {
    if (definedVars.contains(variable.identifier())) {
      continue;
    }
    if (!_namedVariableNodes.contains(variable.identifier())) {
      throw std::runtime_error("Variable with identifier \"" +
                               std::string(variable.identifier()) +
                               "\" is not defined");
    }
    freeVariables.push_back(_namedVariableNodes.at(variable.identifier()));
  }

  if (!freeVariables.empty()) {
    _intVariableNodes.push_back(
        std::make_unique<invariantgraph::InvariantGraphRoot>(freeVariables));
  }
}

std::unique_ptr<invariantgraph::VariableDefiningNode>
invariantgraph::InvariantGraphBuilder::makeVariableDefiningNode(
    const fznparser::Model& _model, const fznparser::Constraint& constraint,
    bool guessDefinedVariable) {
  std::string_view name = constraint.name;

#define NODE_REGISTRATION(nodeType)                                            \
  for (const auto& [nameStr, numArgs] :                                        \
       invariantgraph::nodeType::acceptedNameNumArgPairs()) {                  \
    if (name == nameStr && constraint.arguments.size() == numArgs) {           \
      return invariantgraph::nodeType::fromModelConstraint(_model, constraint, \
                                                           _invariantGraph);   \
    }                                                                          \
  }

  if (!guessDefinedVariable) {
    // For the linear node, we need to know up front what variable is defined.
    NODE_REGISTRATION(IntLinearNode);
    NODE_REGISTRATION(BoolLinearNode);
  }

  NODE_REGISTRATION(ArrayBoolAndNode);
  NODE_REGISTRATION(ArrayBoolElement2dNode);
  NODE_REGISTRATION(ArrayBoolElementNode);
  NODE_REGISTRATION(ArrayBoolOrNode);
  NODE_REGISTRATION(ArrayIntElement2dNode);
  NODE_REGISTRATION(ArrayIntElementNode);
  NODE_REGISTRATION(ArrayIntMaximumNode);
  NODE_REGISTRATION(ArrayIntMinimumNode);
  NODE_REGISTRATION(ArrayVarBoolElement2dNode);
  NODE_REGISTRATION(ArrayVarBoolElementNode);
  NODE_REGISTRATION(ArrayVarIntElement2dNode);
  NODE_REGISTRATION(ArrayVarIntElementNode);
  NODE_REGISTRATION(Bool2IntNode);
  NODE_REGISTRATION(BoolEqNode);
  NODE_REGISTRATION(BoolLeNode);
  NODE_REGISTRATION(BoolLtNode);
  NODE_REGISTRATION(BoolNotNode);
  NODE_REGISTRATION(BoolXorNode);
  NODE_REGISTRATION(IntAbsNode);
  NODE_REGISTRATION(IntDivNode);
  NODE_REGISTRATION(IntEqNode);
  NODE_REGISTRATION(IntLeNode);
  NODE_REGISTRATION(IntLinEqNode);
  NODE_REGISTRATION(IntLinLeNode);
  NODE_REGISTRATION(IntLinNeNode);
  NODE_REGISTRATION(IntLtNode);
  NODE_REGISTRATION(IntMaxNode);
  NODE_REGISTRATION(IntMinNode);
  NODE_REGISTRATION(IntModNode);
  NODE_REGISTRATION(IntNeNode);
  NODE_REGISTRATION(IntPlusNode);
  NODE_REGISTRATION(IntPowNode);
  NODE_REGISTRATION(IntTimesNode);
  NODE_REGISTRATION(SetInNode);

  return nullptr;
#undef BINARY_OP_REGISTRATION
#undef NODE_REGISTRATION
}

std::unique_ptr<invariantgraph::ImplicitConstraintNode>
invariantgraph::InvariantGraphBuilder::makeImplicitConstraint(
    const fznparser::Model& _model, const fznparser::Constraint& constraint) {
  std::string_view name = constraint.name;

#define NODE_REGISTRATION(nodeType)                                            \
  for (const auto& [nameStr, numArgs] :                                        \
       invariantgraph::nodeType::acceptedNameNumArgPairs()) {                  \
    if (name == nameStr && constraint.arguments.size() == numArgs) {           \
      return invariantgraph::nodeType::fromModelConstraint(_model, constraint, \
                                                           _invariantGraph);   \
    }                                                                          \
  }

  NODE_REGISTRATION(AllDifferentImplicitNode);
  NODE_REGISTRATION(CircuitImplicitNode);

  return nullptr;
#undef NODE_REGISTRATION
}

std::unique_ptr<invariantgraph::SoftConstraintNode>
invariantgraph::InvariantGraphBuilder::makeSoftConstraint(
    const fznparser::Model& _model, const fznparser::Constraint& constraint) {
  std::string_view name = constraint.name;

#define NODE_REGISTRATION(nodeType)                                            \
  for (const auto& [nameStr, numArgs] :                                        \
       invariantgraph::nodeType::acceptedNameNumArgPairs()) {                  \
    if (name == nameStr && constraint.arguments.size() == numArgs) {           \
      return invariantgraph::nodeType::fromModelConstraint(_model, constraint, \
                                                           _invariantGraph);   \
    }                                                                          \
  }

  NODE_REGISTRATION(AllDifferentNode);
  NODE_REGISTRATION(AllEqualNode);
  NODE_REGISTRATION(BoolAndNode);
  NODE_REGISTRATION(BoolClauseNode);
  NODE_REGISTRATION(BoolEqNode);
  NODE_REGISTRATION(BoolLeNode);
  NODE_REGISTRATION(BoolLinEqNode);
  NODE_REGISTRATION(BoolLinLeNode);
  NODE_REGISTRATION(BoolLeNode);
  NODE_REGISTRATION(BoolOrNode);
  NODE_REGISTRATION(IntEqNode);
  NODE_REGISTRATION(IntLeNode);
  NODE_REGISTRATION(IntLtNode);
  NODE_REGISTRATION(IntLinEqNode);
  NODE_REGISTRATION(IntLinLeNode);
  NODE_REGISTRATION(IntLinNeNode);
  NODE_REGISTRATION(IntNeNode);
  NODE_REGISTRATION(SetInNode);

  throw std::runtime_error(std::string("Failed to create soft constraint: ")
                               .append(constraint.name));
#undef NODE_REGISTRATION
}

bool invariantGraph::InvariantGraphBuilder::isFreeVariable(
    const fznparser::BoolVar& var,
    const std::unordered_set<std::string_view>& definedVars) {
  return var.isFixed() || !definedVars.contains(constraintArg.identifier());
}

bool invariantGraph::InvariantGraphBuilder::isFreeVariable(
    const fznparser::IntVar& var,
    const std::unordered_set<std::string_view>& definedVars) {
  return var.isFixed() || !definedVars.contains(constraintArg.identifier());
}

bool invariantGraph::InvariantGraphBuilder::argumentIsFreeVariable(
    const fznparser::Arg& constraintArg,
    const std::unordered_set<std::string_view>& definedVars) {
  if (constraintArg.isParameter()) {
    return true;
  }
  if (std::holds_alternative<fznparser::BoolArg>(constraintArg)) {
    return std::holds_alternative<bool>(constraintArg) ||
           isFreeVariable(
               std::get<std::reference_wrapper<const fznparser::BoolVar>>(
                   constraintArg)
                   .get(),
               definedVars);
  } else if (std::holds_alternative<fznparser::IntArg>(constraintArg)) {
    return std::holds_alternative<int64_t>(constraintArg) ||
           isFreeVariable(
               std::get<std::reference_wrapper<const fznparser::IntVar>>(
                   constraintArg)
                   .get(),
               definedVars);
  } else if (std::holds_alternative<BoolVarArray>(constraintArg)) {
    return std::all_of(
        constraintArg.begin(), constraintArg.end(),
        [&](const auto& var) { return isFreeVariable(var, definedVars); });
  } else if (std::holds_alternative<IntVarArray>(constraintArg)) {
    return std::all_of(
        constraintArg.begin(), variable.end(),
        [&](const auto& var) { return isFreeVariable(var, definedVars); });
  }
  return false;
}

bool invariantGraph::InvariantGraphBuilder::allArgumentsAreFreeVariables(
    const fznparser::Constraint& constraint,
    const std::unordered_set<std::string_view>& definedVars) {
  return std::all_of(constraint.arguments.begin(), constraint.arguments.end(),
                     [&](const Arg& arg) {
                       return std::visit<bool>(isFree(arg, definedVars), arg);
                     });
}
