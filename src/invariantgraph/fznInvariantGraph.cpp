#include "invariantgraph/fznInvariantGraph.hpp"

#include "invariantgraph/fzn/allDifferentImplicitNode.hpp"
#include "invariantgraph/fzn/array_bool_and.hpp"
#include "invariantgraph/fzn/array_bool_element.hpp"
#include "invariantgraph/fzn/array_bool_element2d.hpp"
#include "invariantgraph/fzn/array_bool_or.hpp"
#include "invariantgraph/fzn/array_bool_xor.hpp"
#include "invariantgraph/fzn/array_int_element.hpp"
#include "invariantgraph/fzn/array_int_element2d.hpp"
#include "invariantgraph/fzn/array_int_maximum.hpp"
#include "invariantgraph/fzn/array_int_minimum.hpp"
#include "invariantgraph/fzn/array_var_bool_element.hpp"
#include "invariantgraph/fzn/array_var_bool_element2d.hpp"
#include "invariantgraph/fzn/array_var_int_element.hpp"
#include "invariantgraph/fzn/array_var_int_element2d.hpp"
#include "invariantgraph/fzn/bool2int.hpp"
#include "invariantgraph/fzn/bool_and.hpp"
#include "invariantgraph/fzn/bool_clause.hpp"
#include "invariantgraph/fzn/bool_eq.hpp"
#include "invariantgraph/fzn/bool_le.hpp"
#include "invariantgraph/fzn/bool_lin_eq.hpp"
#include "invariantgraph/fzn/bool_lin_le.hpp"
#include "invariantgraph/fzn/bool_lt.hpp"
#include "invariantgraph/fzn/bool_not.hpp"
#include "invariantgraph/fzn/bool_or.hpp"
#include "invariantgraph/fzn/bool_xor.hpp"
#include "invariantgraph/fzn/circuitImplicitNode.hpp"
#include "invariantgraph/fzn/fzn_all_different_int.hpp"
#include "invariantgraph/fzn/fzn_all_equal_int.hpp"
#include "invariantgraph/fzn/fzn_count_eq.hpp"
#include "invariantgraph/fzn/fzn_count_geq.hpp"
#include "invariantgraph/fzn/fzn_count_gt.hpp"
#include "invariantgraph/fzn/fzn_count_leq.hpp"
#include "invariantgraph/fzn/fzn_count_lt.hpp"
#include "invariantgraph/fzn/fzn_count_neq.hpp"
#include "invariantgraph/fzn/fzn_global_cardinality.hpp"
#include "invariantgraph/fzn/fzn_global_cardinality_closed.hpp"
#include "invariantgraph/fzn/fzn_global_cardinality_low_up.hpp"
#include "invariantgraph/fzn/fzn_global_cardinality_low_up_closed.hpp"
#include "invariantgraph/fzn/int_abs.hpp"
#include "invariantgraph/fzn/int_div.hpp"
#include "invariantgraph/fzn/int_eq.hpp"
#include "invariantgraph/fzn/int_le.hpp"
#include "invariantgraph/fzn/int_lin_eq.hpp"
#include "invariantgraph/fzn/int_lin_le.hpp"
#include "invariantgraph/fzn/int_lin_ne.hpp"
#include "invariantgraph/fzn/int_linear.hpp"
#include "invariantgraph/fzn/int_lt.hpp"
#include "invariantgraph/fzn/int_max.hpp"
#include "invariantgraph/fzn/int_min.hpp"
#include "invariantgraph/fzn/int_mod.hpp"
#include "invariantgraph/fzn/int_ne.hpp"
#include "invariantgraph/fzn/int_plus.hpp"
#include "invariantgraph/fzn/int_pow.hpp"
#include "invariantgraph/fzn/int_times.hpp"
#include "invariantgraph/fzn/set_in.hpp"

namespace atlantis::invariantgraph {

FznInvariantGraph::FznInvariantGraph()
    : _outputIdentifiers(),
      _outputBoolVarNodeIds(),
      _outputIntVarNodeIds(),
      _outputBoolVarArrays(),
      _outputIntVarArrays() {}

void FznInvariantGraph::build(const fznparser::Model& model) {
  createNodes(model);

  if (model.hasObjective()) {
    const fznparser::Var& modelObjective = model.objective();
    _objectiveVarNodeId = varNodeId(modelObjective.identifier());
    if (_objectiveVarNodeId == NULL_NODE_ID) {
      if (std::holds_alternative<fznparser::BoolVar>(modelObjective)) {
        _objectiveVarNodeId = createVarNodeFromFzn(
            std::get<fznparser::BoolVar>(modelObjective), true);
      } else if (std::holds_alternative<fznparser::IntVar>(modelObjective)) {
        _objectiveVarNodeId = createVarNodeFromFzn(
            std::get<fznparser::IntVar>(modelObjective), true);
      } else {
        throw FznException("Objective variable is not a BoolVar or IntVar: ");
      }
    }
  }
}

VarNodeId FznInvariantGraph::createVarNodeFromFzn(const fznparser::BoolVar& var,
                                                  bool isDefinedVar) {
  VarNodeId nId = NULL_NODE_ID;
  if (var.isFixed()) {
    if (var.identifier().empty()) {
      nId = createVarNode(var.lowerBound(), isDefinedVar);
    } else {
      nId = createVarNode(var.lowerBound(), var.identifier(), isDefinedVar);
    }
  } else if (var.identifier().empty()) {
    nId = createVarNode(SearchDomain(std::vector<Int>{0, 1}), false,
                        isDefinedVar);
  } else {
    nId = createVarNode(SearchDomain(std::vector<Int>{0, 1}), false,
                        var.identifier(), isDefinedVar);
  }

  if (var.isOutput() && !var.identifier().empty() &&
      !_outputIdentifiers.contains(var.identifier())) {
    _outputIdentifiers.emplace(var.identifier());
    _outputBoolVarNodeIds.emplace_back(nId);
  }

  return nId;
}

VarNodeId FznInvariantGraph::createVarNodeFromFzn(
    std::reference_wrapper<const fznparser::BoolVar> ref, bool isDefinedVar) {
  return createVarNodeFromFzn(ref.get(), isDefinedVar);
}

VarNodeId FznInvariantGraph::createVarNodeFromFzn(const fznparser::BoolArg& arg,
                                                  bool isDefinedVar) {
  return arg.isParameter() ? createVarNode(arg.parameter(), isDefinedVar)
                           : createVarNodeFromFzn(arg.var(), isDefinedVar);
}

VarNodeId FznInvariantGraph::createVarNodeFromFzn(const fznparser::IntVar& var,
                                                  bool isDefinedVar) {
  VarNodeId nId = NULL_NODE_ID;
  if (var.isFixed()) {
    if (var.identifier().empty())
      nId = createVarNode(var.lowerBound(), isDefinedVar);
    else {
      nId = createVarNode(var.lowerBound(), var.identifier(), isDefinedVar);
    }
  } else {
    SearchDomain domain =
        var.domain().isInterval()
            ? SearchDomain(var.domain().lowerBound(), var.domain().upperBound())
            : SearchDomain(var.domain().elements());
    if (var.identifier().empty()) {
      nId = createVarNode(domain, true, isDefinedVar);
    } else {
      nId = createVarNode(domain, true, var.identifier(), isDefinedVar);
    }
  }

  if (var.isOutput() && !var.identifier().empty() &&
      !_outputIdentifiers.contains(var.identifier())) {
    _outputIdentifiers.emplace(var.identifier());
    _outputIntVarNodeIds.emplace_back(nId);
  }

  return nId;
}

VarNodeId FznInvariantGraph::createVarNodeFromFzn(
    std::reference_wrapper<const fznparser::IntVar> ref, bool isDefinedVar) {
  return createVarNodeFromFzn(ref.get(), isDefinedVar);
}

VarNodeId FznInvariantGraph::createVarNodeFromFzn(const fznparser::IntArg& arg,
                                                  bool isDefinedVar) {
  return arg.isParameter()
             ? createVarNode(static_cast<Int>(arg.parameter()), isDefinedVar)
             : createVarNodeFromFzn(arg.var(), isDefinedVar);
}

std::vector<VarNodeId> FznInvariantGraph::createVarNodes(
    const fznparser::BoolVarArray& array, bool areDefinedVars) {
  std::vector<VarNodeId> varNodeIds;
  varNodeIds.reserve(array.size());

  for (size_t i = 0; i < array.size(); ++i) {
    varNodeIds.emplace_back(
        std::holds_alternative<bool>(array.at(i))
            ? createVarNode(std::get<bool>(array.at(i)), areDefinedVars)
            : createVarNodeFromFzn(
                  std::get<std::reference_wrapper<const fznparser::BoolVar>>(
                      array.at(i))
                      .get(),
                  areDefinedVars));
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
    const fznparser::IntVarArray& array, bool areDefinedVars) {
  std::vector<VarNodeId> varNodeIds;
  varNodeIds.reserve(array.size());

  for (size_t i = 0; i < array.size(); ++i) {
    varNodeIds.emplace_back(
        std::holds_alternative<Int>(array.at(i))
            ? createVarNode(std::get<Int>(array.at(i)), areDefinedVars)
            : createVarNodeFromFzn(
                  std::get<std::reference_wrapper<const fznparser::IntVar>>(
                      array.at(i))
                      .get(),
                  areDefinedVars));
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
  for (const VarNodeId& nId : _outputBoolVarNodeIds) {
    const VarNode node = varNodeConst(nId);
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
  for (const VarNodeId& nId : _outputIntVarNodeIds) {
    const VarNode node = varNodeConst(nId);
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
    for (const VarNodeId& nId : outputArray.varNodeIds) {
      const VarNode& node = varNodeConst(nId);
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
    for (const VarNodeId& nId : outputArray.varNodeIds) {
      const VarNode& node = varNodeConst(nId);
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

void FznInvariantGraph::createNodes(const fznparser::Model& model) {
  std::unordered_set<std::string> definedVars;
  std::vector<bool> constraintIsProcessed(model.constraints().size(), false);

  std::vector<std::function<bool(const fznparser::Constraint&)>>
      invariantNodeCreators{
          [&](const fznparser::Constraint& c) {
            return makeImplicitConstraintNode(c);
          },
          [&](const fznparser::Constraint& c) { return makeInvariantNode(c); },
          [&](const fznparser::Constraint& c) {
            return makeViolationInvariantNode(c);
          }};

  for (const auto& invNodeCreator : invariantNodeCreators) {
    for (size_t idx = 0; idx < model.constraints().size(); ++idx) {
      const fznparser::Constraint& constraint = model.constraints().at(idx);
      if (constraintIsProcessed.at(idx)) {
        continue;
      }
      if (invNodeCreator(constraint)) {
        constraintIsProcessed.at(idx) = true;
      }
    }
  }
  for (size_t i = 0; i < constraintIsProcessed.size(); ++i) {
    if (!constraintIsProcessed.at(i)) {
      throw FznException(
          std::string("Failed to create invariant node for constraint: ")
              .append(model.constraints().at(i).identifier()));
    }
  }

  assert(std::all_of(constraintIsProcessed.begin(), constraintIsProcessed.end(),
                     [](bool b) { return b; }));

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

bool FznInvariantGraph::makeInvariantNode(
    const fznparser::Constraint& constraint, bool guessDefinedVar) {
  std::string name = constraint.identifier();

#define MAKE_INVARIANT(fznConstraintName)     \
  if (fznConstraintName(*this, constraint)) { \
    return true;                              \
  }

  if (!guessDefinedVar) {
    // For the linear node, we need to know up front what variable is
    // defined.
    MAKE_INVARIANT(fzn::int_linear);
  }

  MAKE_INVARIANT(fzn::array_bool_and);
  MAKE_INVARIANT(fzn::array_bool_element2d);
  MAKE_INVARIANT(fzn::array_bool_element);
  MAKE_INVARIANT(fzn::array_bool_or);
  MAKE_INVARIANT(fzn::array_int_element2d);
  MAKE_INVARIANT(fzn::array_int_element);
  MAKE_INVARIANT(fzn::array_int_maximum);
  MAKE_INVARIANT(fzn::array_int_minimum);
  MAKE_INVARIANT(fzn::array_var_bool_element2d);
  MAKE_INVARIANT(fzn::array_var_bool_element);
  MAKE_INVARIANT(fzn::array_var_int_element2d);
  MAKE_INVARIANT(fzn::array_var_int_element);
  MAKE_INVARIANT(fzn::bool2int);
  MAKE_INVARIANT(fzn::bool_eq);
  MAKE_INVARIANT(fzn::bool_le);
  MAKE_INVARIANT(fzn::bool_lt);
  MAKE_INVARIANT(fzn::bool_not);
  MAKE_INVARIANT(fzn::bool_xor);
  MAKE_INVARIANT(fzn::int_abs);
  MAKE_INVARIANT(fzn::int_div);
  MAKE_INVARIANT(fzn::int_eq);
  MAKE_INVARIANT(fzn::int_le);
  MAKE_INVARIANT(fzn::int_lin_eq);
  MAKE_INVARIANT(fzn::int_lin_le);
  MAKE_INVARIANT(fzn::int_lin_ne);
  MAKE_INVARIANT(fzn::int_lt);
  MAKE_INVARIANT(fzn::int_max);
  MAKE_INVARIANT(fzn::int_min);
  MAKE_INVARIANT(fzn::int_mod);
  MAKE_INVARIANT(fzn::int_ne);
  MAKE_INVARIANT(fzn::int_plus);
  MAKE_INVARIANT(fzn::int_pow);
  MAKE_INVARIANT(fzn::int_times);
  MAKE_INVARIANT(fzn::set_in);

  return false;
#undef MAKE_INVARIANT
}

bool FznInvariantGraph::makeImplicitConstraintNode(
    const fznparser::Constraint& constraint) {
  std::string name = constraint.identifier();

#define MAKE_IMPLICIT_CONSTRAINT(fznConstraintName) \
  if (fznConstraintName(*this, constraint)) {       \
    return true;                                    \
  }

  MAKE_IMPLICIT_CONSTRAINT(fzn::makeAllDifferentImplicitNode);
  MAKE_IMPLICIT_CONSTRAINT(fzn::makeCircuitImplicitNode);

  return false;
#undef MAKE_IMPLICIT_CONSTRAINT
}

bool FznInvariantGraph::makeViolationInvariantNode(
    const fznparser::Constraint& constraint) {
  std::string name = constraint.identifier();

#define MAKE_VIOLATION_INVARIANT(fznConstraintName) \
  if (fznConstraintName(*this, constraint)) {       \
    return true;                                    \
  }

  MAKE_VIOLATION_INVARIANT(fzn::bool_and);
  MAKE_VIOLATION_INVARIANT(fzn::bool_clause);
  MAKE_VIOLATION_INVARIANT(fzn::bool_eq);
  MAKE_VIOLATION_INVARIANT(fzn::bool_le);
  MAKE_VIOLATION_INVARIANT(fzn::bool_le);
  MAKE_VIOLATION_INVARIANT(fzn::bool_lin_eq);
  MAKE_VIOLATION_INVARIANT(fzn::bool_lin_le);
  MAKE_VIOLATION_INVARIANT(fzn::bool_or);
  MAKE_VIOLATION_INVARIANT(fzn::fzn_all_different_int);
  MAKE_VIOLATION_INVARIANT(fzn::int_eq);
  MAKE_VIOLATION_INVARIANT(fzn::int_le);
  MAKE_VIOLATION_INVARIANT(fzn::int_lin_eq);
  MAKE_VIOLATION_INVARIANT(fzn::int_lin_le);
  MAKE_VIOLATION_INVARIANT(fzn::int_lin_ne);
  MAKE_VIOLATION_INVARIANT(fzn::int_lt);
  MAKE_VIOLATION_INVARIANT(fzn::int_ne);
  MAKE_VIOLATION_INVARIANT(fzn::set_in);

  throw std::runtime_error(std::string("Failed to create soft constraint: ")
                               .append(constraint.identifier()));
#undef MAKE_VIOLATION_INVARIANT
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

}  // namespace atlantis::invariantgraph