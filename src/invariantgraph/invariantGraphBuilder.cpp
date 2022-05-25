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
#include "invariantgraph/invariants/arrayBoolElementNode.hpp"
#include "invariantgraph/invariants/arrayIntElementNode.hpp"
#include "invariantgraph/invariants/arrayIntMaximumNode.hpp"
#include "invariantgraph/invariants/arrayIntMinimumNode.hpp"
#include "invariantgraph/invariants/arrayVarBoolElementNode.hpp"
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

invariantgraph::InvariantGraph invariantgraph::InvariantGraphBuilder::build(
    const fznparser::FZNModel& model) {
  _variableMap.clear();
  _variables.clear();
  _definingNodes.clear();

  createNodes(model);

  // clang-format off
  auto objectiveVariable = std::visit<invariantgraph::VariableNode*>(
      overloaded{
        [](const fznparser::Satisfy&) { return nullptr; },
        [&](const auto& objective) {
          return _variableMap.at(objective.variable);
        }
      },
      model.objective());
  // clang-format on

  std::vector<VariableNode*> valueNodes;
  for (const auto& [_, node] : _valueMap) {
    valueNodes.push_back(node);
  }

  return {std::move(_variables), std::move(valueNodes),
          std::move(_definingNodes), objectiveVariable};
}

using FZNSearchVariable =
    std::variant<fznparser::IntVariable, fznparser::BoolVariable>;

static std::optional<FZNSearchVariable> searchVariable(
    const fznparser::Variable& variable) {
  return std::visit<std::optional<FZNSearchVariable>>(
      overloaded{[](const fznparser::IntVariable& var) { return var; },
                 [](const fznparser::BoolVariable& var) { return var; },
                 [](const auto&) { return std::nullopt; }},
      variable);
}

static bool allVariablesFree(
    const fznparser::Constraint& constraint,
    const std::unordered_set<fznparser::Identifier>& definedVars);

static void markVariablesAsDefined(
    const invariantgraph::VariableDefiningNode& node,
    std::unordered_set<fznparser::Identifier>& definedVars) {
  for (const auto& variableNode : node.definedVariables()) {
    if (variableNode->variable()) {
      definedVars.emplace(identifier(*variableNode->variable()));
    }
  }
}

void invariantgraph::InvariantGraphBuilder::createNodes(
    const fznparser::FZNModel& model) {
  for (const auto& variable : model.variables()) {
    auto var = searchVariable(variable);
    if (!var) {
      continue;
    }

    auto variableNode = std::visit<std::unique_ptr<VariableNode>>(
        [](const auto& v) { return std::make_unique<VariableNode>(v); }, *var);

    _variableMap.emplace(identifier(variable), variableNode.get());
    _variables.push_back(std::move(variableNode));
  }

  std::unordered_set<fznparser::Identifier> definedVars;
  std::unordered_set<size_t> processedConstraints;

  // First, define implicit constraints (neighborhood)
  for (size_t idx = 0; idx < model.constraints().size(); ++idx) {
    auto constraint = model.constraints()[idx];

    auto allVariablesAreFree = allVariablesFree(constraint, definedVars);
    if (processedConstraints.contains(idx) || !allVariablesAreFree) {
      continue;
    }

    if (auto implicitConstraint = makeImplicitConstraint(model, constraint)) {
      for (auto variableNode : implicitConstraint->definedVariables()) {
        definedVars.emplace(identifier(*variableNode->variable()));
      }

      _definingNodes.push_back(std::move(implicitConstraint));
      processedConstraints.emplace(idx);
    }
  }

  // Second, define variables based on annotations.
  for (size_t idx = 0; idx < model.constraints().size(); ++idx) {
    auto constraint = model.constraints()[idx];

    auto annotation =
        getAnnotation<fznparser::DefinesVariableAnnotation>(constraint);
    if (!annotation) {
      continue;
    }

    if (definedVars.count(annotation->definedVariable)) {
      continue;
    }

    if (auto node = makeVariableDefiningNode(model, constraint)) {
      markVariablesAsDefined(*node, definedVars);

      _definingNodes.push_back(std::move(node));
      processedConstraints.emplace(idx);
    }
  }

  // Third, pick up any invariants we can identify without the annotations
  for (size_t idx = 0; idx < model.constraints().size(); ++idx) {
    auto constraint = model.constraints()[idx];
    if (processedConstraints.count(idx)) {
      continue;
    }

    if (auto node = makeVariableDefiningNode(model, constraint, true)) {
      auto canBeInvariant = std::all_of(
          node->definedVariables().begin(), node->definedVariables().end(),
          [&](const auto& variableNode) {
            if (!variableNode->variable()) {
              return true;
            }

            return definedVars.count(identifier(*variableNode->variable())) ==
                   0;
          });

      if (!canBeInvariant) {
        continue;
      }

      markVariablesAsDefined(*node, definedVars);

      _definingNodes.push_back(std::move(node));
      processedConstraints.emplace(idx);
    }
  }

  // Fourth, use soft constraints for the remaining model constraints.
  for (size_t idx = 0; idx < model.constraints().size(); ++idx) {
    auto constraint = model.constraints()[idx];
    if (processedConstraints.count(idx)) {
      continue;
    }

    _definingNodes.push_back(makeSoftConstraint(model, constraint));
    processedConstraints.emplace(idx);
  }

  assert(processedConstraints.size() == model.constraints().size());

  // Finally, define all free variables by the InvariantGraphRoot
  std::vector<VariableNode*> freeVariables;
  for (const auto& variable : model.variables()) {
    if (std::holds_alternative<fznparser::IntVariable>(variable) ||
        std::holds_alternative<fznparser::BoolVariable>(variable)) {
      auto variableName = identifier(variable);
      if (definedVars.count(variableName) == 0) {
        freeVariables.push_back(_variableMap.at(variableName));
      }
    }
  }

  if (!freeVariables.empty()) {
    _definingNodes.push_back(
        std::make_unique<invariantgraph::InvariantGraphRoot>(freeVariables));
  }
}

std::unique_ptr<invariantgraph::VariableDefiningNode>
invariantgraph::InvariantGraphBuilder::makeVariableDefiningNode(
    const fznparser::FZNModel& model, const fznparser::Constraint& constraint,
    bool guessDefinedVariable) {
  std::string_view name = constraint.name;

#define NODE_REGISTRATION(nodeType)                                            \
  for (const auto& [nameStr, numArgs] :                                        \
       invariantgraph::nodeType::acceptedNameNumArgPairs()) {                  \
    if (name == nameStr && constraint.arguments.size() == numArgs) {           \
      return invariantgraph::nodeType::fromModelConstraint(                    \
          model, constraint,                                                   \
          [&](const auto& argument) { return nodeFactory(model, argument); }); \
    }                                                                          \
  }

  if (!guessDefinedVariable) {
    // For the linear node, we need to know up front what variable is defined.
    NODE_REGISTRATION(IntLinearNode);
    NODE_REGISTRATION(BoolLinearNode);
  }

  NODE_REGISTRATION(IntDivNode);
  NODE_REGISTRATION(IntMaxNode);
  NODE_REGISTRATION(IntMinNode);
  NODE_REGISTRATION(IntModNode);
  NODE_REGISTRATION(IntPlusNode);
  NODE_REGISTRATION(IntPowNode);
  NODE_REGISTRATION(IntTimesNode);
  NODE_REGISTRATION(ArrayBoolAndNode);
  NODE_REGISTRATION(ArrayBoolElementNode);
  NODE_REGISTRATION(ArrayBoolOrNode);
  NODE_REGISTRATION(ArrayIntElementNode);
  NODE_REGISTRATION(ArrayIntMaximumNode);
  NODE_REGISTRATION(ArrayIntMinimumNode);
  NODE_REGISTRATION(ArrayVarBoolElementNode);
  NODE_REGISTRATION(ArrayVarIntElementNode);
  NODE_REGISTRATION(Bool2IntNode);
  NODE_REGISTRATION(BoolEqNode);
  NODE_REGISTRATION(BoolLeNode);
  NODE_REGISTRATION(BoolLtNode);
  NODE_REGISTRATION(BoolXorNode);
  NODE_REGISTRATION(IntAbsNode);
  NODE_REGISTRATION(IntEqNode);
  NODE_REGISTRATION(IntLeNode);
  NODE_REGISTRATION(IntLinEqNode);
  NODE_REGISTRATION(IntLinLeNode);
  NODE_REGISTRATION(IntLinNeNode);
  NODE_REGISTRATION(IntLtNode);
  NODE_REGISTRATION(IntNeNode);
  NODE_REGISTRATION(SetInNode);

  return nullptr;
#undef BINARY_OP_REGISTRATION
#undef NODE_REGISTRATION
}

std::unique_ptr<invariantgraph::ImplicitConstraintNode>
invariantgraph::InvariantGraphBuilder::makeImplicitConstraint(
    const fznparser::FZNModel& model, const fznparser::Constraint& constraint) {
  std::string_view name = constraint.name;

#define NODE_REGISTRATION(nodeType)                                            \
  for (const auto& [nameStr, numArgs] :                                        \
       invariantgraph::nodeType::acceptedNameNumArgPairs()) {                  \
    if (name == nameStr && constraint.arguments.size() == numArgs) {           \
      return invariantgraph::nodeType::fromModelConstraint(                    \
          model, constraint,                                                   \
          [&](const auto& argument) { return nodeFactory(model, argument); }); \
    }                                                                          \
  }

  NODE_REGISTRATION(AllDifferentImplicitNode);
  NODE_REGISTRATION(CircuitImplicitNode);

  return nullptr;
#undef NODE_REGISTRATION
}

std::unique_ptr<invariantgraph::SoftConstraintNode>
invariantgraph::InvariantGraphBuilder::makeSoftConstraint(
    const fznparser::FZNModel& model, const fznparser::Constraint& constraint) {
  std::string_view name = constraint.name;

#define NODE_REGISTRATION(nodeType)                                            \
  for (const auto& [nameStr, numArgs] :                                        \
       invariantgraph::nodeType::acceptedNameNumArgPairs()) {                  \
    if (name == nameStr && constraint.arguments.size() == numArgs) {           \
      return invariantgraph::nodeType::fromModelConstraint(                    \
          model, constraint,                                                   \
          [&](const auto& argument) { return nodeFactory(model, argument); }); \
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

invariantgraph::VariableNode*
invariantgraph::InvariantGraphBuilder::nodeFactory(
    const fznparser::FZNModel& model, const MappableValue& argument) {
  return std::visit<VariableNode*>(
      overloaded{[&](Int value) { return nodeForValue(value); },
                 [&](bool value) { return nodeForValue(value); },
                 [&](const fznparser::Identifier& identifier) {
                   return nodeForIdentifier(model, identifier);
                 }},
      argument);
}

invariantgraph::VariableNode*
invariantgraph::InvariantGraphBuilder::nodeForIdentifier(
    const fznparser::FZNModel& model, const fznparser::Identifier& identifier) {
  auto item = model.identify(identifier);
  assert(item);

  // clang-format off
  return std::visit<VariableNode*>(overloaded{
      [&](const fznparser::Parameter& parameter) {
        return nodeForParameter(parameter);
      },
      [&](const fznparser::Variable&) {
        return _variableMap.at(identifier);
      }
  }, *item);
  // clang-format on
}

invariantgraph::VariableNode*
invariantgraph::InvariantGraphBuilder::nodeForParameter(
    const fznparser::Parameter& parameter) {
  return std::visit<VariableNode*>(
      overloaded{
          [&](const fznparser::IntParameter& intParam) {
            return nodeForValue(intParam.value);
          },
          [&](const fznparser::BoolParameter& boolParam) {
            return nodeForValue(boolParam.value);
          },

          // Shouldn't happen. See comment near the top of the header file.
          [](const auto&) {
            assert(false);
            return nullptr;
          }},
      parameter);
}

static bool allVariablesFree(
    const fznparser::Constraint& constraint,
    const std::unordered_set<fznparser::Identifier>& definedVars) {
  auto isFree = [&](const fznparser::Identifier& identifier) {
    return !definedVars.contains(identifier);
  };

  return std::all_of(
      constraint.arguments.begin(), constraint.arguments.end(),
      [&](const auto& arg) {
        return std::visit<bool>(
            overloaded{isFree,
                       [&](const fznparser::Constraint::ArrayArgument& array) {
                         return std::all_of(
                             array.begin(), array.end(),
                             [&](const auto& element) {
                               return std::visit<bool>(
                                   overloaded{isFree,
                                              [](const auto&) { return true; }},
                                   element);
                             });
                       },
                       [](const auto&) { return false; }},
            arg);
      });
}
