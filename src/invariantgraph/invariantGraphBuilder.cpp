#include "invariantgraph/invariantGraphBuilder.hpp"

#include <unordered_set>

#include "invariantgraph/constraints/allDifferentNode.hpp"
#include "invariantgraph/constraints/boolClauseNode.hpp"
#include "invariantgraph/constraints/eqNode.hpp"
#include "invariantgraph/constraints/intLinNeNode.hpp"
#include "invariantgraph/constraints/intNeNode.hpp"
#include "invariantgraph/constraints/linEqNode.hpp"
#include "invariantgraph/constraints/linLeNode.hpp"
#include "invariantgraph/constraints/setInNode.hpp"
#include "invariantgraph/implicitConstraints/allDifferentImplicitNode.hpp"
#include "invariantgraph/invariantGraphRoot.hpp"
#include "invariantgraph/invariants/arrayBoolAndNode.hpp"
#include "invariantgraph/invariants/arrayBoolElementNode.hpp"
#include "invariantgraph/invariants/arrayBoolOrNode.hpp"
#include "invariantgraph/invariants/arrayIntElementNode.hpp"
#include "invariantgraph/invariants/arrayVarBoolElementNode.hpp"
#include "invariantgraph/invariants/arrayVarIntElementNode.hpp"
#include "invariantgraph/invariants/binaryOpNode.hpp"
#include "invariantgraph/invariants/intDivNode.hpp"
#include "invariantgraph/invariants/intModNode.hpp"
#include "invariantgraph/invariants/intPowNode.hpp"
#include "invariantgraph/invariants/intTimesNode.hpp"
#include "invariantgraph/invariants/linearNode.hpp"
#include "invariantgraph/invariants/maxNode.hpp"
#include "invariantgraph/invariants/minNode.hpp"
#include "invariantgraph/views/bool2IntNode.hpp"
#include "invariantgraph/views/boolNotNode.hpp"
#include "invariantgraph/views/eqReifNode.hpp"
#include "invariantgraph/views/intAbsNode.hpp"
#include "invariantgraph/views/intLinEqReifNode.hpp"
#include "invariantgraph/views/intLinLeReifNode.hpp"
#include "invariantgraph/views/intLinNeReifNode.hpp"
#include "invariantgraph/views/intNeReifNode.hpp"
#include "invariantgraph/views/leReifNode.hpp"
#include "invariantgraph/views/ltReifNode.hpp"
#include "invariantgraph/views/setInReifNode.hpp"
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

  return {std::move(_variables), std::move(_definingNodes), objectiveVariable};
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

  // First, define based on annotations.
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

  // Second, define an implicit constraint (neighborhood) on remaining
  // constraints.
  for (size_t idx = 0; idx < model.constraints().size(); ++idx) {
    auto constraint = model.constraints()[idx];

    if (processedConstraints.count(idx) ||
        !allVariablesFree(constraint, definedVars)) {
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
  }

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

#define NODE_REGISTRATION(nameStr, nodeType)            \
  if (name == nameStr)                                  \
  return invariantgraph::nodeType::fromModelConstraint( \
      model, constraint,                                \
      [&](const auto& argument) { return nodeFactory(model, argument); })

#define BINARY_OP_REGISTRATION(nodeType)                                       \
  if (name == invariantgraph::nodeType::constraint_name())                     \
  return invariantgraph::BinaryOpNode::fromModelConstraint<                    \
      invariantgraph::nodeType>(model, constraint, [&](const auto& argument) { \
    return nodeFactory(model, argument);                                       \
  })

  if (!guessDefinedVariable) {
    // For the linear node, we need to know up front what variable is defined.
    NODE_REGISTRATION("int_lin_eq", LinearNode);
  }

  NODE_REGISTRATION("int_abs", IntAbsNode);
  NODE_REGISTRATION("array_int_maximum", MaxNode);
  NODE_REGISTRATION("array_int_minimum", MinNode);
  NODE_REGISTRATION("array_int_element", ArrayIntElementNode);
  NODE_REGISTRATION("array_var_int_element", ArrayVarIntElementNode);
  BINARY_OP_REGISTRATION(IntDivNode);
  BINARY_OP_REGISTRATION(IntModNode);
  BINARY_OP_REGISTRATION(IntTimesNode);
  BINARY_OP_REGISTRATION(IntPowNode);
  NODE_REGISTRATION("int_abs", IntAbsNode);
  NODE_REGISTRATION("int_eq_reif", EqReifNode);
  NODE_REGISTRATION("bool_eq_reif", EqReifNode);
  NODE_REGISTRATION("int_le_reif", LeReifNode);
  NODE_REGISTRATION("bool_le_reif", LeReifNode);
  NODE_REGISTRATION("int_lin_eq_reif", IntLinEqReifNode);
  NODE_REGISTRATION("int_lin_le_reif", IntLinLeReifNode);
  NODE_REGISTRATION("int_lin_ne_reif", IntLinNeReifNode);
  NODE_REGISTRATION("int_lt_reif", LtReifNode);
  NODE_REGISTRATION("bool_lt_reif", LtReifNode);
  NODE_REGISTRATION("int_ne_reif", IntNeReifNode);
  NODE_REGISTRATION("set_in_reif", SetInReifNode);
  NODE_REGISTRATION("bool2int", Bool2IntNode);
  NODE_REGISTRATION("bool_not", BoolNotNode);
  NODE_REGISTRATION("array_bool_and", ArrayBoolAndNode);
  NODE_REGISTRATION("array_bool_or", ArrayBoolOrNode);
  NODE_REGISTRATION("array_bool_element", ArrayBoolElementNode);
  NODE_REGISTRATION("array_var_bool_element", ArrayVarBoolElementNode);

  return nullptr;
#undef BINARY_OP_REGISTRATION
#undef NODE_REGISTRATION
}

std::unique_ptr<invariantgraph::ImplicitConstraintNode>
invariantgraph::InvariantGraphBuilder::makeImplicitConstraint(
    const fznparser::FZNModel& model, const fznparser::Constraint& constraint) {
  std::string_view name = constraint.name;

#define NODE_REGISTRATION(nameStr, nodeType)            \
  if (name == nameStr)                                  \
  return invariantgraph::nodeType::fromModelConstraint( \
      model, constraint,                                \
      [&](const auto& argument) { return nodeFactory(model, argument); })

  NODE_REGISTRATION("alldifferent", AllDifferentImplicitNode);

  return nullptr;
#undef NODE_REGISTRATION
}

std::unique_ptr<invariantgraph::SoftConstraintNode>
invariantgraph::InvariantGraphBuilder::makeSoftConstraint(
    const fznparser::FZNModel& model, const fznparser::Constraint& constraint) {
  std::string_view name = constraint.name;

#define NODE_REGISTRATION(nameStr, nodeType)            \
  if (name == nameStr)                                  \
  return invariantgraph::nodeType::fromModelConstraint( \
      model, constraint,                                \
      [&](const auto& argument) { return nodeFactory(model, argument); })

  NODE_REGISTRATION("alldifferent", AllDifferentNode);
  NODE_REGISTRATION("int_lin_le", LinLeNode);
  NODE_REGISTRATION("bool_lin_le", LinLeNode);
  NODE_REGISTRATION("int_lin_eq", LinEqNode);
  NODE_REGISTRATION("bool_lin_eq", LinEqNode);
  NODE_REGISTRATION("int_lin_ne", IntLinNeNode);
  NODE_REGISTRATION("int_eq", EqNode);
  NODE_REGISTRATION("bool_eq", EqNode);
  NODE_REGISTRATION("int_ne", IntNeNode);
  NODE_REGISTRATION("set_in", SetInNode);
  NODE_REGISTRATION("bool_clause", BoolClauseNode);

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
    return definedVars.count(identifier) > 0;
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
