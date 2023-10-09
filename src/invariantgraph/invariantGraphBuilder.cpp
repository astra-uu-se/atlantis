#include "invariantgraph/invariantGraphBuilder.hpp"

#include <unordered_set>

#include "invariantgraph/implicitConstraints/allDifferentImplicitNode.hpp"
#include "invariantgraph/implicitConstraints/circuitImplicitNode.hpp"
#include "invariantgraph/invariantGraphRoot.hpp"
#include "invariantgraph/invariantNodes/arrayBoolElement2dNode.hpp"
#include "invariantgraph/invariantNodes/arrayBoolElementNode.hpp"
#include "invariantgraph/invariantNodes/arrayIntElement2dNode.hpp"
#include "invariantgraph/invariantNodes/arrayIntElementNode.hpp"
#include "invariantgraph/invariantNodes/arrayIntMaximumNode.hpp"
#include "invariantgraph/invariantNodes/arrayIntMinimumNode.hpp"
#include "invariantgraph/invariantNodes/arrayVarBoolElement2dNode.hpp"
#include "invariantgraph/invariantNodes/arrayVarBoolElementNode.hpp"
#include "invariantgraph/invariantNodes/arrayVarIntElement2dNode.hpp"
#include "invariantgraph/invariantNodes/arrayVarIntElementNode.hpp"
#include "invariantgraph/invariantNodes/boolLinearNode.hpp"
#include "invariantgraph/invariantNodes/intDivNode.hpp"
#include "invariantgraph/invariantNodes/intLinearNode.hpp"
#include "invariantgraph/invariantNodes/intMaxNode.hpp"
#include "invariantgraph/invariantNodes/intMinNode.hpp"
#include "invariantgraph/invariantNodes/intModNode.hpp"
#include "invariantgraph/invariantNodes/intPlusNode.hpp"
#include "invariantgraph/invariantNodes/intPowNode.hpp"
#include "invariantgraph/invariantNodes/intTimesNode.hpp"
#include "invariantgraph/views/bool2IntNode.hpp"
#include "invariantgraph/views/boolNotNode.hpp"
#include "invariantgraph/views/intAbsNode.hpp"
#include "invariantgraph/violationInvariantNodes/allDifferentNode.hpp"
#include "invariantgraph/violationInvariantNodes/allEqualNode.hpp"
#include "invariantgraph/violationInvariantNodes/arrayBoolAndNode.hpp"
#include "invariantgraph/violationInvariantNodes/arrayBoolOrNode.hpp"
#include "invariantgraph/violationInvariantNodes/boolAndNode.hpp"
#include "invariantgraph/violationInvariantNodes/boolClauseNode.hpp"
#include "invariantgraph/violationInvariantNodes/boolEqNode.hpp"
#include "invariantgraph/violationInvariantNodes/boolLeNode.hpp"
#include "invariantgraph/violationInvariantNodes/boolLinEqNode.hpp"
#include "invariantgraph/violationInvariantNodes/boolLinLeNode.hpp"
#include "invariantgraph/violationInvariantNodes/boolLtNode.hpp"
#include "invariantgraph/violationInvariantNodes/boolOrNode.hpp"
#include "invariantgraph/violationInvariantNodes/boolXorNode.hpp"
#include "invariantgraph/violationInvariantNodes/intEqNode.hpp"
#include "invariantgraph/violationInvariantNodes/intLeNode.hpp"
#include "invariantgraph/violationInvariantNodes/intLinEqNode.hpp"
#include "invariantgraph/violationInvariantNodes/intLinLeNode.hpp"
#include "invariantgraph/violationInvariantNodes/intLinNeNode.hpp"
#include "invariantgraph/violationInvariantNodes/intLtNode.hpp"
#include "invariantgraph/violationInvariantNodes/intNeNode.hpp"
#include "invariantgraph/violationInvariantNodes/setInNode.hpp"
#include "utils/fznAst.hpp"

invariantgraph::InvariantGraphBuilder::InvariantGraphBuilder(
    fznparser::Model& _model)
    : _model(_model), _invariantGraph() {}

invariantgraph::InvariantGraph invariantgraph::InvariantGraphBuilder::build() {
  initVariableNodes();

  createNodes();

  const std::optional<fznparser::Variable> objectiveVariable =
      _model.objective();
  return std::move(_invariantGraph);
}

using FZNSearchVariable = std::variant<fznparser::IntVar, fznparser::BoolVar>;

bool argumentIsFree(const fznparser::Arg& arg,
                    const std::unordered_set<std::string>& definedVars);

bool argumentsAreFree(const fznparser::Constraint& constraint,
                      const std::unordered_set<std::string>& definedVars);

void invariantgraph::InvariantGraphBuilder::markOutputTo(
    const invariantgraph::InvariantNode& invNode,
    std::unordered_set<std::string>& definedVars) {
  for (invariantgraph::VarNodeId varNodeId : invNode.outputVarNodeIds()) {
    assert(_invariantGraph.varNode(varNodeId).definingNodes().contains(
        invNode.id()));
    const std::string& identifier =
        _invariantGraph.varNode(varNodeId).identifier();
    if (!identifier.empty() && !definedVars.contains(identifier)) {
      definedVars.emplace(identifier);
    }
  }
}

void invariantgraph::InvariantGraphBuilder::initVariableNodes() {
  for (const auto& [identifier, variable] : _model.variables()) {
    if (std::holds_alternative<fznparser::BoolVar>(variable)) {
      _invariantGraph.createVarNode(std::get<fznparser::BoolVar>(variable));
    } else if (std::holds_alternative<fznparser::IntVar>(variable)) {
      _invariantGraph.createVarNode(std::get<fznparser::IntVar>(variable));
    }
  }
}

void invariantgraph::InvariantGraphBuilder::createNodes() {
  std::unordered_set<std::string> definedVars;
  std::vector<bool> constraintIsProcessed(_model.constraints().size(), false);

  // First, define variables based on annotations.
  for (size_t idx = 0; idx < _model.constraints().size(); ++idx) {
    auto& constraint = _model.constraints().at(idx);

    std::optional<std::reference_wrapper<const fznparser::Variable>>
        definedVar = constraint.definedVariable();

    if (!definedVar.has_value()) {
      continue;
    }

    const fznparser::Variable& variable = definedVar.value().get();

    if (!std::holds_alternative<fznparser::BoolVar>(variable) ||
        std::holds_alternative<fznparser::IntVar>(variable)) {
      continue;
    }

    if (definedVars.count(variable.identifier())) {
      continue;
    }

    std::unique_ptr<InvariantNode> invNode = makeInvariantNode(constraint);

    if (invNode == nullptr) {
      continue;
    }

    invariantgraph::InvariantNodeId invId =
        _invariantGraph.addInvariantNode(std::move(invNode));

    markOutputTo(_invariantGraph.invariantNode(invId), definedVars);

    _invariantGraph.invariantNode(invId).prune(_invariantGraph);

    constraintIsProcessed.at(idx) = true;
  }

  // Second, define implicit constraints (neighborhood)
  for (size_t idx = 0; idx < _model.constraints().size(); ++idx) {
    const fznparser::Constraint& constraint = _model.constraints().at(idx);

    if (constraintIsProcessed.at(idx) ||
        !argumentsAreFree(constraint, definedVars)) {
      continue;
    }

    std::unique_ptr<ImplicitConstraintNode> implicitConstraintNode =
        makeImplicitConstraintNode(constraint);
    if (implicitConstraintNode == nullptr) {
      continue;
    }

    invariantgraph::InvariantNodeId implNodeId =
        _invariantGraph.addImplicitConstraintNode(
            std::move(implicitConstraintNode));

    markOutputTo(_invariantGraph.implicitConstraintNode(implNodeId),
                 definedVars);

    _invariantGraph.implicitConstraintNode(implNodeId).prune(_invariantGraph);

    constraintIsProcessed.at(idx) = true;
  }

  // Third, pick up any invariants we can identify without the annotations
  for (size_t idx = 0; idx < _model.constraints().size(); ++idx) {
    const fznparser::Constraint& constraint = _model.constraints().at(idx);
    if (constraintIsProcessed.at(idx)) {
      continue;
    }
    std::unique_ptr<invariantgraph::InvariantNode> invNode =
        makeInvariantNode(constraint, true);

    if (invNode == nullptr) {
      continue;
    }

    /*
    // If some variable is defined by multiple invariants, then _invariantGraph
    // will split the variables

    bool canBeInvariant = std::all_of(
        invNode->outputVarNodeIds().begin(), invNode->outputVarNodeIds().end(),
        [&](const auto& varNodeId) {
          return _invariantGraph.varNode(varNodeId).variable().has_value() &&
                 definedVars.contains(
                     _invariantGraph.varNode(varNodeId).identifier());
        });

    if (!canBeInvariant) {
      continue;
    }
    */

    const InvariantNodeId invNodeId =
        _invariantGraph.addInvariantNode(std::move(invNode));

    _invariantGraph.invariantNode(invNodeId).prune(_invariantGraph);

    markOutputTo(_invariantGraph.invariantNode(invNodeId), definedVars);

    constraintIsProcessed.at(idx) = true;
  }

  // Fourth, use soft constraints for the remaining _model constraints.
  for (size_t idx = 0; idx < _model.constraints().size(); ++idx) {
    const fznparser::Constraint& constraint = _model.constraints().at(idx);
    if (constraintIsProcessed.at(idx)) {
      continue;
    }

    std::unique_ptr<invariantgraph::ViolationInvariantNode>
        violationInvariantNode = makeViolationInvariantNode(constraint);

    if (violationInvariantNode == nullptr) {
      throw std::runtime_error("Failed to create soft constraint: " +
                               std::string(constraint.identifier()));
    }

    invariantgraph::InvariantNodeId invNodeId =
        _invariantGraph.addInvariantNode(std::move(violationInvariantNode));

    _invariantGraph.invariantNode(invNodeId).prune(_invariantGraph);

    constraintIsProcessed.at(idx) = true;
  }

  assert(std::all_of(constraintIsProcessed.begin(), constraintIsProcessed.end(),
                     [](bool b) { return b; }));

  // Finally, define all free variables by the InvariantGraphRoot
  for (const auto& [identifier, variable] : _model.variables()) {
    assert(!identifier.empty());
    if (std::holds_alternative<fznparser::IntVar>(variable)) {
      const fznparser::IntVar& intVar = std::get<fznparser::IntVar>(variable);
      if (intVar.isFixed()) {
        continue;
      }
      assert(_invariantGraph.varNode(identifier).varNodeId() !=
             invariantgraph::NULL_NODE_ID);
      assert(_invariantGraph.varNode(identifier).isIntVar());
    } else if (std::holds_alternative<fznparser::BoolVar>(variable)) {
      const fznparser::BoolVar& boolVar =
          std::get<fznparser::BoolVar>(variable);
      if (boolVar.isFixed()) {
        continue;
      }
      assert(_invariantGraph.varNode(identifier).varNodeId() !=
             invariantgraph::NULL_NODE_ID);
      assert(!_invariantGraph.varNode(identifier).isIntVar());
    }
  }
}

std::unique_ptr<invariantgraph::InvariantNode>
invariantgraph::InvariantGraphBuilder::makeInvariantNode(
    const fznparser::Constraint& constraint, bool guessDefinedVariable) {
  std::string name = constraint.identifier();

#define NODE_REGISTRATION(nodeType)                                          \
  for (const auto& [nameStr, numArgs] :                                      \
       invariantgraph::nodeType::acceptedNameNumArgPairs()) {                \
    if (name == nameStr && constraint.arguments().size() == numArgs) {       \
      return invariantgraph::nodeType::fromModelConstraint(constraint,       \
                                                           _invariantGraph); \
    }                                                                        \
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
invariantgraph::InvariantGraphBuilder::makeImplicitConstraintNode(
    const fznparser::Constraint& constraint) {
  std::string name = constraint.identifier();

#define NODE_REGISTRATION(nodeType)                                          \
  for (const auto& [nameStr, numArgs] :                                      \
       invariantgraph::nodeType::acceptedNameNumArgPairs()) {                \
    if (name == nameStr && constraint.arguments().size() == numArgs) {       \
      return invariantgraph::nodeType::fromModelConstraint(constraint,       \
                                                           _invariantGraph); \
    }                                                                        \
  }

  NODE_REGISTRATION(AllDifferentImplicitNode);
  NODE_REGISTRATION(CircuitImplicitNode);

  return nullptr;
#undef NODE_REGISTRATION
}

std::unique_ptr<invariantgraph::ViolationInvariantNode>
invariantgraph::InvariantGraphBuilder::makeViolationInvariantNode(
    const fznparser::Constraint& constraint) {
  std::string name = constraint.identifier();

#define NODE_REGISTRATION(nodeType)                                          \
  for (const auto& [nameStr, numArgs] :                                      \
       invariantgraph::nodeType::acceptedNameNumArgPairs()) {                \
    if (name == nameStr && constraint.arguments().size() == numArgs) {       \
      return invariantgraph::nodeType::fromModelConstraint(constraint,       \
                                                           _invariantGraph); \
    }                                                                        \
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
                               .append(constraint.identifier()));
#undef NODE_REGISTRATION
}

bool varIsFree(const fznparser::BoolVar& var,
               const std::unordered_set<std::string>& definedVars) {
  return var.isFixed() || !definedVars.contains(var.identifier());
}
bool varIsFree(const fznparser::IntVar& var,
               const std::unordered_set<std::string>& definedVars) {
  return var.isFixed() || !definedVars.contains(var.identifier());
}
bool varIsFree(const fznparser::FloatVar& var,
               const std::unordered_set<std::string>&) {
  return var.isFixed();
}
bool varIsFree(const fznparser::SetVar&,
               const std::unordered_set<std::string>&) {
  return false;
}
bool argIsFree(const fznparser::BoolArg& arg,
               const std::unordered_set<std::string>& definedVars) {
  return arg.isParameter() || varIsFree(arg.var(), definedVars);
}
bool argIsFree(const fznparser::IntArg& arg,
               const std::unordered_set<std::string>& definedVars) {
  return arg.isParameter() || varIsFree(arg.var(), definedVars);
}
bool argIsFree(const fznparser::FloatArg& arg,
               const std::unordered_set<std::string>& definedVars) {
  return arg.isParameter() || varIsFree(arg.var(), definedVars);
}
bool argIsFree(const fznparser::IntSetArg& arg,
               const std::unordered_set<std::string>& definedVars) {
  return arg.isParameter() || varIsFree(arg.var(), definedVars);
}

template <typename ArrayType, typename VarType, typename ParType>
bool arrayIsFree(const ArrayType& var,
                 const std::unordered_set<std::string>& definedVars) {
  for (size_t i = 0; i < var.size(); ++i) {
    if (!std::holds_alternative<ParType>(var.at(i)) &&
        !argIsFree(
            std::get<std::reference_wrapper<const VarType>>(var.at(i)).get(),
            definedVars)) {
      return false;
    }
  }
  return true;
}

bool argumentIsFree(const fznparser::Arg& arg,
                    const std::unordered_set<std::string>& definedVars) {
  if (std::holds_alternative<fznparser::BoolArg>(arg)) {
    return argIsFree(std::get<fznparser::BoolArg>(arg), definedVars);
  } else if (std::holds_alternative<fznparser::IntArg>(arg)) {
    return argIsFree(std::get<fznparser::IntArg>(arg), definedVars);
  } else if (std::holds_alternative<fznparser::FloatArg>(arg)) {
    return argIsFree(std::get<fznparser::FloatArg>(arg), definedVars);
  } else if (std::holds_alternative<fznparser::IntSetArg>(arg)) {
    return argIsFree(std::get<fznparser::IntSetArg>(arg), definedVars);
  } else if (std::holds_alternative<fznparser::BoolVarArray>(arg)) {
    return arrayIsFree<fznparser::BoolVarArray, fznparser::BoolVar, bool>(
        std::get<fznparser::BoolVarArray>(arg), definedVars);
  } else if (std::holds_alternative<fznparser::IntVarArray>(arg)) {
    return arrayIsFree<fznparser::IntVarArray, fznparser::IntVar, Int>(
        std::get<fznparser::IntVarArray>(arg), definedVars);
  } else if (std::holds_alternative<fznparser::FloatVarArray>(arg)) {
    return arrayIsFree<fznparser::FloatVarArray, fznparser::FloatVar, double>(
        std::get<fznparser::FloatVarArray>(arg), definedVars);
  } else if (std::holds_alternative<fznparser::SetVarArray>(arg)) {
    return arrayIsFree<fznparser::SetVarArray, fznparser::SetVar,
                       fznparser::IntSet>(std::get<fznparser::SetVarArray>(arg),
                                          definedVars);
  }
  return false;
}

bool argumentsAreFree(const fznparser::Constraint& constraint,
                      const std::unordered_set<std::string>& definedVars) {
  return std::all_of(constraint.arguments().begin(),
                     constraint.arguments().end(),
                     [&](const fznparser::Arg& arg) {
                       return argumentIsFree(arg, definedVars);
                     });
}
