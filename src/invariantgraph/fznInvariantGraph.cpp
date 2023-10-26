#include "invariantgraph/fznInvariantGraph.hpp"

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

namespace atlantis::invariantgraph {

FznInvariantGraph::FznInvariantGraph()
    : _outputIdentifiers(),
      _outputBoolVarNodeIds(),
      _outputIntVarNodeIds(),
      _outputBoolVarArrays(),
      _outputIntVarArrays() {}

void FznInvariantGraph::build(const fznparser::Model& model) {
  initVarNodes(model);

  createNodes(model);

  if (model.objective().has_value()) {
    const fznparser::Var& modelObjective = model.objective().value();
    if (std::holds_alternative<fznparser::BoolVar>(modelObjective)) {
      _objectiveVarNodeId =
          createVarNode(std::get<fznparser::BoolVar>(modelObjective));
    } else if (std::holds_alternative<fznparser::IntVar>(modelObjective)) {
      _objectiveVarNodeId =
          createVarNode(std::get<fznparser::IntVar>(modelObjective));
    }
  }
}

VarNodeId FznInvariantGraph::createVarNode(const fznparser::BoolVar& var) {
  VarNodeId nodeId = NULL_NODE_ID;
  if (!var.identifier().empty() && containsVarNode(var.identifier())) {
    nodeId = varNode(var.identifier()).varNodeId();
  } else if (var.isFixed()) {
    nodeId = InvariantGraph::createVarNode(var.lowerBound());
  } else {
    nodeId = InvariantGraph::createVarNode(SearchDomain(std::vector<Int>{0, 1}),
                                           false, var.identifier());
  }

  if (var.isOutput() && !var.identifier().empty() &&
      !_outputIdentifiers.contains(var.identifier())) {
    _outputIdentifiers.emplace(var.identifier());
    _outputBoolVarNodeIds.emplace_back(nodeId);
  }

  return nodeId;
}

VarNodeId FznInvariantGraph::createVarNode(
    std::reference_wrapper<const fznparser::BoolVar> ref) {
  return createVarNode(ref.get());
}

VarNodeId FznInvariantGraph::createVarNode(const fznparser::BoolArg& arg) {
  return arg.isParameter() ? InvariantGraph::createVarNode(arg.parameter())
                           : createVarNode(arg.var());
}

VarNodeId FznInvariantGraph::createVarNode(const fznparser::IntVar& var) {
  VarNodeId nodeId = NULL_NODE_ID;
  if (!var.identifier().empty() && containsVarNode(var.identifier())) {
    nodeId = varNode(var.identifier()).varNodeId();
  } else if (var.isFixed()) {
    nodeId = createVarNode(var.lowerBound());
  } else {
    SearchDomain domain =
        var.domain().isInterval()
            ? SearchDomain(var.domain().lowerBound(), var.domain().upperBound())
            : SearchDomain(var.domain().elements());

    nodeId = InvariantGraph::createVarNode(domain, true, var.identifier());
  }

  if (var.isOutput() && !var.identifier().empty() &&
      !_outputIdentifiers.contains(var.identifier())) {
    _outputIdentifiers.emplace(var.identifier());
    _outputIntVarNodeIds.emplace_back(nodeId);
  }

  return nodeId;
}

VarNodeId FznInvariantGraph::createVarNode(
    std::reference_wrapper<const fznparser::IntVar> ref) {
  return createVarNode(ref.get());
}

VarNodeId FznInvariantGraph::createVarNode(const fznparser::IntArg& arg) {
  return arg.isParameter()
             ? InvariantGraph::createVarNode(static_cast<Int>(arg.parameter()))
             : createVarNode(arg.var());
}

std::vector<VarNodeId> FznInvariantGraph::createVarNodes(
    const fznparser::BoolVarArray& array) {
  std::vector<VarNodeId> varNodeIds;
  varNodeIds.reserve(array.size());

  for (size_t i = 0; i < array.size(); ++i) {
    varNodeIds.emplace_back(
        std::holds_alternative<bool>(array.at(i))
            ? InvariantGraph::createVarNode(std::get<bool>(array.at(i)))
            : createVarNode(
                  std::get<std::reference_wrapper<const fznparser::BoolVar>>(
                      array.at(i))
                      .get()));
  }

  if (array.isOutput() && !array.identifier().empty() &&
      !_outputIdentifiers.contains(array.identifier())) {
    _outputIdentifiers.emplace(array.identifier());
    _outputBoolVarArrays.emplace_back(array.identifier(),
                                      array.outputIndexSetSizes(), varNodeIds);
  }

  return varNodeIds;
}

std::vector<VarNodeId> FznInvariantGraph::createVarNodes(
    const fznparser::IntVarArray& array) {
  std::vector<VarNodeId> varNodeIds;
  varNodeIds.reserve(array.size());

  for (size_t i = 0; i < array.size(); ++i) {
    varNodeIds.emplace_back(
        std::holds_alternative<Int>(array.at(i))
            ? createVarNode(std::get<Int>(array.at(i)))
            : createVarNode(
                  std::get<std::reference_wrapper<const fznparser::IntVar>>(
                      array.at(i))
                      .get()));
  }

  if (array.isOutput() && !array.identifier().empty() &&
      !_outputIdentifiers.contains(array.identifier())) {
    _outputIdentifiers.emplace(array.identifier());
    _outputIntVarArrays.emplace_back(array.identifier(),
                                     array.outputIndexSetSizes(), varNodeIds);
  }

  return varNodeIds;
}

std::vector<FznOutputVar> FznInvariantGraph::outputBoolVars() const noexcept {
  std::vector<FznOutputVar> outputVars;
  outputVars.reserve(_outputBoolVarNodeIds.size());
  for (const VarNodeId& nodeId : _outputBoolVarNodeIds) {
    const VarNode node = varNodeConst(nodeId);
    if (node.isFixed()) {
      outputVars.emplace_back(node.identifier(), node.constantValue().value());
    } else {
      outputVars.emplace_back(node.identifier(), node.varId());
    }
  }
  return outputVars;
}

std::vector<FznOutputVar> FznInvariantGraph::outputIntVars() const noexcept {
  std::vector<FznOutputVar> outputVars;
  outputVars.reserve(_outputIntVarNodeIds.size());
  for (const VarNodeId& nodeId : _outputIntVarNodeIds) {
    const VarNode node = varNodeConst(nodeId);
    if (node.isFixed()) {
      outputVars.emplace_back(node.identifier(), node.constantValue().value());
    } else {
      outputVars.emplace_back(node.identifier(), node.varId());
    }
  }
  return outputVars;
}
std::vector<FznOutputVarArray> FznInvariantGraph::outputBoolVarArrays()
    const noexcept {
  std::vector<FznOutputVarArray> outputVarArrays;
  outputVarArrays.reserve(_outputBoolVarArrays.size());
  for (const InvariantGraphOutputVarArray& outputArray : _outputBoolVarArrays) {
    FznOutputVarArray& fznArray =
        outputVarArrays.emplace_back(FznOutputVarArray{
            outputArray.identifier, outputArray.indexSetSizes, {}});
    fznArray.vars.reserve(outputArray.varNodeIds.size());
    for (const VarNodeId& nodeId : outputArray.varNodeIds) {
      const VarNode& node = varNodeConst(nodeId);
      if (node.isFixed()) {
        fznArray.vars.emplace_back(node.constantValue().value());
      } else {
        fznArray.vars.emplace_back(node.varId());
      }
    }
  }
  return outputVarArrays;
}
std::vector<FznOutputVarArray> FznInvariantGraph::outputIntVarArrays()
    const noexcept {
  std::vector<FznOutputVarArray> outputVarArrays;
  outputVarArrays.reserve(_outputIntVarArrays.size());
  for (const InvariantGraphOutputVarArray& outputArray : _outputIntVarArrays) {
    FznOutputVarArray& fznArray =
        outputVarArrays.emplace_back(FznOutputVarArray{
            outputArray.identifier, outputArray.indexSetSizes, {}});
    fznArray.vars.reserve(outputArray.varNodeIds.size());
    for (const VarNodeId& nodeId : outputArray.varNodeIds) {
      const VarNode& node = varNodeConst(nodeId);
      if (node.isFixed()) {
        fznArray.vars.emplace_back(node.constantValue().value());
      } else {
        fznArray.vars.emplace_back(node.varId());
      }
    }
  }
  return outputVarArrays;
}

bool argumentIsFree(const fznparser::Arg& arg,
                    const std::unordered_set<std::string>& definedVars);

bool argumentsAreFree(const fznparser::Constraint& constraint,
                      const std::unordered_set<std::string>& definedVars);

void FznInvariantGraph::markOutputTo(
    const InvariantNode& invNode,
    std::unordered_set<std::string>& definedVars) {
  for (VarNodeId varNodeId : invNode.outputVarNodeIds()) {
    assert(varNode(varNodeId).definingNodes().contains(invNode.id()));
    const std::string& identifier = varNode(varNodeId).identifier();
    if (!identifier.empty() && !definedVars.contains(identifier)) {
      definedVars.emplace(identifier);
    }
  }
}

void FznInvariantGraph::initVarNodes(const fznparser::Model& model) {
  for (const auto& [identifier, var] : model.vars()) {
    if (std::holds_alternative<fznparser::BoolVar>(var)) {
      createVarNode(std::get<fznparser::BoolVar>(var));
    } else if (std::holds_alternative<fznparser::IntVar>(var)) {
      createVarNode(std::get<fznparser::IntVar>(var));
    } else if (std::holds_alternative<fznparser::BoolVarArray>(var)) {
      const fznparser::BoolVarArray& boolVarArray =
          std::get<fznparser::BoolVarArray>(var);
      if (boolVarArray.isOutput()) {
        createVarNodes(boolVarArray);
      }
    } else if (std::holds_alternative<fznparser::IntVarArray>(var)) {
      const fznparser::IntVarArray& intVarArray =
          std::get<fznparser::IntVarArray>(var);
      if (intVarArray.isOutput()) {
        createVarNodes(intVarArray);
      }
    }
  }
}

void FznInvariantGraph::createNodes(const fznparser::Model& model) {
  std::unordered_set<std::string> definedVars;
  std::vector<bool> constraintIsProcessed(model.constraints().size(), false);

  // First, define variables based on annotations.
  for (size_t idx = 0; idx < model.constraints().size(); ++idx) {
    auto& constraint = model.constraints().at(idx);

    std::optional<std::reference_wrapper<const fznparser::Var>> definedVar =
        constraint.definedVar();

    if (!definedVar.has_value()) {
      continue;
    }

    const fznparser::Var& var = definedVar.value().get();

    if (!std::holds_alternative<fznparser::BoolVar>(var) ||
        std::holds_alternative<fznparser::IntVar>(var)) {
      continue;
    }

    if (definedVars.count(var.identifier())) {
      continue;
    }

    std::unique_ptr<InvariantNode> invNode = makeInvariantNode(constraint);

    if (invNode == nullptr) {
      continue;
    }

    InvariantNodeId invId = addInvariantNode(std::move(invNode));

    markOutputTo(invariantNode(invId), definedVars);

    invariantNode(invId).prune(*this);

    constraintIsProcessed.at(idx) = true;
  }

  // Second, define implicit constraints (neighborhood)
  for (size_t idx = 0; idx < model.constraints().size(); ++idx) {
    const fznparser::Constraint& constraint = model.constraints().at(idx);

    if (constraintIsProcessed.at(idx) ||
        !argumentsAreFree(constraint, definedVars)) {
      continue;
    }

    std::unique_ptr<ImplicitConstraintNode> impConNode =
        makeImplicitConstraintNode(constraint);
    if (impConNode == nullptr) {
      continue;
    }

    InvariantNodeId implNodeId =
        addImplicitConstraintNode(std::move(impConNode));

    markOutputTo(implicitConstraintNode(implNodeId), definedVars);

    implicitConstraintNode(implNodeId).prune(*this);

    constraintIsProcessed.at(idx) = true;
  }

  // Third, pick up any invariants we can identify without the
  // annotations
  for (size_t idx = 0; idx < model.constraints().size(); ++idx) {
    const fznparser::Constraint& constraint = model.constraints().at(idx);
    if (constraintIsProcessed.at(idx)) {
      continue;
    }
    std::unique_ptr<InvariantNode> invNode =
        makeInvariantNode(constraint, true);

    if (invNode == nullptr) {
      continue;
    }

    /*
    // If some variable is defined by multiple invariants, then
    _invariantGraph
    // will split the variables

    bool canBeInvariant = std::all_of(
        invNode->outputVarNodeIds().begin(),
    invNode->outputVarNodeIds().end(),
        [&](const auto& varNodeId) {
          return varNode(varNodeId).var().has_value() &&
                 definedVars.contains(
                     varNode(varNodeId).identifier());
        });

    if (!canBeInvariant) {
      continue;
    }
    */

    const InvariantNodeId invNodeId = addInvariantNode(std::move(invNode));

    invariantNode(invNodeId).prune(*this);

    markOutputTo(invariantNode(invNodeId), definedVars);

    constraintIsProcessed.at(idx) = true;
  }

  // Fourth, use soft constraints for the remaining model constraints.
  for (size_t idx = 0; idx < model.constraints().size(); ++idx) {
    const fznparser::Constraint& constraint = model.constraints().at(idx);
    if (constraintIsProcessed.at(idx)) {
      continue;
    }

    std::unique_ptr<ViolationInvariantNode> violationInvariantNode =
        makeViolationInvariantNode(constraint);

    if (violationInvariantNode == nullptr) {
      throw std::runtime_error("Failed to create soft constraint: " +
                               std::string(constraint.identifier()));
    }

    InvariantNodeId invNodeId =
        addInvariantNode(std::move(violationInvariantNode));

    invariantNode(invNodeId).prune(*this);

    constraintIsProcessed.at(idx) = true;
  }

  assert(std::all_of(constraintIsProcessed.begin(), constraintIsProcessed.end(),
                     [](bool b) { return b; }));

  // Finally, define all free variables by the InvariantGraphRoot
  for (const auto& [identifier, var] : model.vars()) {
    assert(!identifier.empty());
    if (std::holds_alternative<fznparser::IntVar>(var)) {
      const fznparser::IntVar& intVar = std::get<fznparser::IntVar>(var);
      if (intVar.isFixed()) {
        continue;
      }
      assert(varNode(identifier).varNodeId() != NULL_NODE_ID);
      assert(varNode(identifier).isIntVar());
    } else if (std::holds_alternative<fznparser::BoolVar>(var)) {
      const fznparser::BoolVar& boolVar = std::get<fznparser::BoolVar>(var);
      if (boolVar.isFixed()) {
        continue;
      }
      assert(varNode(identifier).varNodeId() != NULL_NODE_ID);
      assert(!varNode(identifier).isIntVar());
    }
  }
}

std::unique_ptr<InvariantNode> FznInvariantGraph::makeInvariantNode(
    const fznparser::Constraint& constraint, bool guessDefinedVar) {
  std::string name = constraint.identifier();

#define NODE_REGISTRATION(nodeType)                                            \
  for (const auto& [nameStr, numArgs] : nodeType::acceptedNameNumArgPairs()) { \
    if (name == nameStr && constraint.arguments().size() == numArgs) {         \
      return nodeType::fromModelConstraint(constraint, *this);                 \
    }                                                                          \
  }

  if (!guessDefinedVar) {
    // For the linear node, we need to know up front what variable is
    // defined.
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

std::unique_ptr<ImplicitConstraintNode>
FznInvariantGraph::makeImplicitConstraintNode(
    const fznparser::Constraint& constraint) {
  std::string name = constraint.identifier();

#define NODE_REGISTRATION(nodeType)                                            \
  for (const auto& [nameStr, numArgs] : nodeType::acceptedNameNumArgPairs()) { \
    if (name == nameStr && constraint.arguments().size() == numArgs) {         \
      return nodeType::fromModelConstraint(constraint, *this);                 \
    }                                                                          \
  }

  NODE_REGISTRATION(AllDifferentImplicitNode);
  NODE_REGISTRATION(CircuitImplicitNode);

  return nullptr;
#undef NODE_REGISTRATION
}

std::unique_ptr<ViolationInvariantNode>
FznInvariantGraph::makeViolationInvariantNode(
    const fznparser::Constraint& constraint) {
  std::string name = constraint.identifier();

#define NODE_REGISTRATION(nodeType)                                            \
  for (const auto& [nameStr, numArgs] : nodeType::acceptedNameNumArgPairs()) { \
    if (name == nameStr && constraint.arguments().size() == numArgs) {         \
      return nodeType::fromModelConstraint(constraint, *this);                 \
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

}  // namespace atlantis::invariantgraph