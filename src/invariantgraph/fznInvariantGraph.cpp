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
        _objectiveVarNodeId =
            defineVarNode(std::get<fznparser::BoolVar>(modelObjective));
      } else if (std::holds_alternative<fznparser::IntVar>(modelObjective)) {
        _objectiveVarNodeId =
            defineVarNode(std::get<fznparser::IntVar>(modelObjective));
      } else {
        throw FznException("Objective variable is not a BoolVar or IntVar");
      }
    }
  }
}

VarNodeId FznInvariantGraph::inputVarNode(const fznparser::BoolVar& var) {
  VarNodeId nId(NULL_NODE_ID);
  if (var.isFixed()) {
    nId = inputBoolVarNode(var.lowerBound());
  } else if (!var.identifier().empty()) {
    nId = inputBoolVarNode(var.identifier());
  } else {
    throw FznException(
        "Input BoolVar must be a parameter or have an identifier");
  }

  if (var.isOutput() && !var.identifier().empty() &&
      !_outputIdentifiers.contains(var.identifier())) {
    _outputIdentifiers.emplace(var.identifier());
    _outputBoolVarNodeIds.emplace_back(nId);
  }

  return nId;
}

VarNodeId FznInvariantGraph::inputVarNode(
    std::reference_wrapper<const fznparser::BoolVar> ref) {
  return inputVarNode(ref.get());
}

VarNodeId FznInvariantGraph::inputVarNode(const fznparser::BoolArg& arg) {
  return arg.isParameter() ? inputBoolVarNode(arg.parameter())
                           : inputVarNode(arg.var());
}

VarNodeId FznInvariantGraph::defineVarNode(const fznparser::BoolVar& var) {
  VarNodeId nId(NULL_NODE_ID);
  if (var.isFixed()) {
    nId = var.identifier().empty()
              ? defineBoolVarNode(var.lowerBound())
              : defineBoolVarNode(var.lowerBound(), var.identifier());
  } else if (!var.identifier().empty()) {
    nId = defineBoolVarNode(var.identifier());
  } else {
    throw FznException(
        "Input BoolVar must be a parameter or have an identifier");
  }

  if (var.isOutput() && !var.identifier().empty() &&
      !_outputIdentifiers.contains(var.identifier())) {
    _outputIdentifiers.emplace(var.identifier());
    _outputBoolVarNodeIds.emplace_back(nId);
  }
  return nId;
}

VarNodeId FznInvariantGraph::defineVarNode(
    std::reference_wrapper<const fznparser::BoolVar> ref) {
  return defineVarNode(ref.get());
}

VarNodeId FznInvariantGraph::defineVarNode(const fznparser::BoolArg& arg) {
  return arg.isParameter() ? defineBoolVarNode(arg.parameter())
                           : defineVarNode(arg.var());
}

VarNodeId FznInvariantGraph::inputVarNode(const fznparser::IntVar& var) {
  VarNodeId nId(NULL_NODE_ID);
  if (var.isFixed()) {
    nId = inputIntVarNode(var.lowerBound());
  } else if (!var.identifier().empty()) {
    nId = inputIntVarNode(var.identifier());
  } else {
    throw FznException(
        "Input IntVar must be a parameter or have an identifier");
  }

  if (var.isOutput() && !var.identifier().empty() &&
      !_outputIdentifiers.contains(var.identifier())) {
    _outputIdentifiers.emplace(var.identifier());
    _outputIntVarNodeIds.emplace_back(nId);
  }

  return nId;
}

VarNodeId FznInvariantGraph::inputVarNode(
    std::reference_wrapper<const fznparser::IntVar> ref) {
  return inputVarNode(ref.get());
}

VarNodeId FznInvariantGraph::inputVarNode(const fznparser::IntArg& arg) {
  return arg.isParameter() ? inputIntVarNode(arg.parameter())
                           : inputVarNode(arg.var());
}

VarNodeId FznInvariantGraph::defineVarNode(const fznparser::IntVar& var) {
  VarNodeId nId(NULL_NODE_ID);
  if (var.isFixed()) {
    nId = var.identifier().empty()
              ? defineIntVarNode(var.lowerBound())
              : defineIntVarNode(var.lowerBound(), var.identifier());
  } else if (!var.identifier().empty()) {
    nId = defineIntVarNode(
        var.domain().isInterval()
            ? SearchDomain(var.domain().lowerBound(), var.domain().upperBound())
            : SearchDomain(var.domain().elements()),
        var.identifier());
  } else {
    throw FznException(
        "Input IntVar must be a parameter or have an identifier");
  }

  if (var.isOutput() && !var.identifier().empty() &&
      !_outputIdentifiers.contains(var.identifier())) {
    _outputIdentifiers.emplace(var.identifier());
    _outputIntVarNodeIds.emplace_back(nId);
  }

  return nId;
}

VarNodeId FznInvariantGraph::defineVarNode(
    std::reference_wrapper<const fznparser::IntVar> ref) {
  return defineVarNode(ref.get());
}

VarNodeId FznInvariantGraph::defineVarNode(const fznparser::IntArg& arg) {
  return arg.isParameter() ? defineIntVarNode(static_cast<Int>(arg.parameter()))
                           : defineVarNode(arg.var());
}

std::vector<VarNodeId> FznInvariantGraph::inputVarNodes(
    const fznparser::BoolVarArray& array) {
  std::vector<VarNodeId> varNodeIds;
  varNodeIds.reserve(array.size());

  for (size_t i = 0; i < array.size(); ++i) {
    varNodeIds.emplace_back(
        std::holds_alternative<bool>(array.at(i))
            ? inputBoolVarNode(std::get<bool>(array.at(i)))
            : defineVarNode(
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

std::vector<VarNodeId> FznInvariantGraph::defineVarNodes(
    const fznparser::BoolVarArray& array) {
  std::vector<VarNodeId> varNodeIds;
  varNodeIds.reserve(array.size());

  for (size_t i = 0; i < array.size(); ++i) {
    varNodeIds.emplace_back(
        std::holds_alternative<bool>(array.at(i))
            ? defineBoolVarNode(std::get<bool>(array.at(i)))
            : defineVarNode(
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

std::vector<VarNodeId> FznInvariantGraph::inputVarNodes(
    const fznparser::IntVarArray& array) {
  std::vector<VarNodeId> varNodeIds;
  varNodeIds.reserve(array.size());

  for (size_t i = 0; i < array.size(); ++i) {
    varNodeIds.emplace_back(
        std::holds_alternative<Int>(array.at(i))
            ? inputIntVarNode(std::get<Int>(array.at(i)))
            : defineVarNode(
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

std::vector<VarNodeId> FznInvariantGraph::defineVarNodes(
    const fznparser::IntVarArray& array) {
  std::vector<VarNodeId> varNodeIds;
  varNodeIds.reserve(array.size());

  for (size_t i = 0; i < array.size(); ++i) {
    varNodeIds.emplace_back(
        std::holds_alternative<Int>(array.at(i))
            ? defineIntVarNode(std::get<Int>(array.at(i)))
            : defineVarNode(
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
  for (const VarNodeId& nId : _outputBoolVarNodeIds) {
    const VarNode node = varNodeConst(nId);
    if (node.isFixed()) {
      outputVars.emplace_back(node.identifier(), node.constantValue().value());
    } else {
      outputVars.emplace_back(
          node.identifier(),
          node.varId());
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
      outputVars.emplace_back(
          node.identifier(),
          node.varId());
    }
  }
  return outputVars;
}

std::vector<FznOutputVarArray> FznInvariantGraph::outputBoolVarArrays()
    const noexcept {
  std::vector<FznOutputVarArray> outputVarArrays;
  outputVarArrays.reserve(_outputBoolVarArrays.size());
  for (const InvariantGraphOutputVarArray& outputArray : _outputBoolVarArrays) {
    FznOutputVarArray& fznArray = outputVarArrays.emplace_back(
        std::string(outputArray.identifier),
        std::vector<Int>(outputArray.indexSetSizes));
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
    FznOutputVarArray& fznArray = outputVarArrays.emplace_back(
        std::string(outputArray.identifier),
        std::vector<Int>(outputArray.indexSetSizes));
    fznArray.vars.reserve(outputArray.varNodeIds.size());
    for (const VarNodeId& nId : outputArray.varNodeIds) {
      const VarNode& node = varNodeConst(nId);
      if (node.hasDomain() && node.isFixed()) {
        fznArray.vars.emplace_back(node.constantValue().value());
      } else {
        fznArray.vars.emplace_back(node.varId());
      }
    }
  }
  return outputVarArrays;
}

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
      const auto& intVar = std::get<fznparser::IntVar>(var);
      if (intVar.isFixed()) {
        continue;
      }
      assert(varNode(identifier).varNodeId() != NULL_NODE_ID);
      assert(varNode(identifier).isIntVar());
    } else if (std::holds_alternative<fznparser::BoolVar>(var)) {
      const auto& boolVar = std::get<fznparser::BoolVar>(var);
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
#define MAKE_INVARIANT(fznConstraintName)     \
  if (fznConstraintName(*this, constraint)) { \
    return true;                              \
  }

  if (!guessDefinedVar) {
    // For the linear node, we need to know up front what variable is
    // defined.
    MAKE_INVARIANT(fzn::int_linear)
  }

  MAKE_INVARIANT(fzn::array_bool_and)
  MAKE_INVARIANT(fzn::array_bool_element2d)
  MAKE_INVARIANT(fzn::array_bool_element)
  MAKE_INVARIANT(fzn::array_bool_or)
  MAKE_INVARIANT(fzn::array_bool_xor)
  MAKE_INVARIANT(fzn::array_int_element2d)
  MAKE_INVARIANT(fzn::array_int_element)
  MAKE_INVARIANT(fzn::array_int_maximum)
  MAKE_INVARIANT(fzn::array_int_minimum)
  MAKE_INVARIANT(fzn::array_var_bool_element2d)
  MAKE_INVARIANT(fzn::array_var_bool_element)
  MAKE_INVARIANT(fzn::array_var_int_element2d)
  MAKE_INVARIANT(fzn::array_var_int_element)
  MAKE_INVARIANT(fzn::bool2int)
  MAKE_INVARIANT(fzn::bool_and)
  MAKE_INVARIANT(fzn::bool_eq)
  MAKE_INVARIANT(fzn::bool_le)
  MAKE_INVARIANT(fzn::bool_lt)
  MAKE_INVARIANT(fzn::bool_not)
  MAKE_INVARIANT(fzn::bool_or)
  MAKE_INVARIANT(fzn::bool_xor)
  MAKE_INVARIANT(fzn::fzn_count_eq)
  MAKE_INVARIANT(fzn::fzn_global_cardinality)
  MAKE_INVARIANT(fzn::fzn_global_cardinality_closed)
  MAKE_INVARIANT(fzn::int_abs)
  MAKE_INVARIANT(fzn::int_div)
  MAKE_INVARIANT(fzn::int_eq)
  MAKE_INVARIANT(fzn::int_le)
  MAKE_INVARIANT(fzn::int_lin_eq)
  MAKE_INVARIANT(fzn::int_lin_le)
  MAKE_INVARIANT(fzn::int_lin_ne)
  MAKE_INVARIANT(fzn::int_lt)
  MAKE_INVARIANT(fzn::int_max)
  MAKE_INVARIANT(fzn::int_min)
  MAKE_INVARIANT(fzn::int_mod)
  MAKE_INVARIANT(fzn::int_ne)
  MAKE_INVARIANT(fzn::int_plus)
  MAKE_INVARIANT(fzn::int_pow)
  MAKE_INVARIANT(fzn::int_times)
  MAKE_INVARIANT(fzn::set_in)

  return false;
#undef MAKE_INVARIANT
}

bool FznInvariantGraph::makeImplicitConstraintNode(
    const fznparser::Constraint& constraint) {
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
#define MAKE_VIOLATION_INVARIANT(fznConstraintName) \
  if (fznConstraintName(*this, constraint)) {       \
    return true;                                    \
  }

  MAKE_VIOLATION_INVARIANT(fzn::bool_and)
  MAKE_VIOLATION_INVARIANT(fzn::bool_clause)
  MAKE_VIOLATION_INVARIANT(fzn::bool_eq)
  MAKE_VIOLATION_INVARIANT(fzn::bool_le)
  MAKE_VIOLATION_INVARIANT(fzn::bool_le)
  MAKE_VIOLATION_INVARIANT(fzn::bool_lin_eq)
  MAKE_VIOLATION_INVARIANT(fzn::bool_lin_le)
  MAKE_VIOLATION_INVARIANT(fzn::bool_or)
  MAKE_VIOLATION_INVARIANT(fzn::fzn_all_different_int)
  MAKE_VIOLATION_INVARIANT(fzn::fzn_all_equal_int)
  MAKE_VIOLATION_INVARIANT(fzn::fzn_count_geq)
  MAKE_VIOLATION_INVARIANT(fzn::fzn_count_gt)
  MAKE_VIOLATION_INVARIANT(fzn::fzn_count_leq)
  MAKE_VIOLATION_INVARIANT(fzn::fzn_count_lt)
  MAKE_VIOLATION_INVARIANT(fzn::fzn_count_neq)
  MAKE_VIOLATION_INVARIANT(fzn::fzn_global_cardinality)
  MAKE_VIOLATION_INVARIANT(fzn::fzn_global_cardinality_closed)
  MAKE_VIOLATION_INVARIANT(fzn::fzn_global_cardinality_low_up)
  MAKE_VIOLATION_INVARIANT(fzn::fzn_global_cardinality_low_up_closed)
  MAKE_VIOLATION_INVARIANT(fzn::int_eq)
  MAKE_VIOLATION_INVARIANT(fzn::int_le)
  MAKE_VIOLATION_INVARIANT(fzn::int_lin_eq)
  MAKE_VIOLATION_INVARIANT(fzn::int_lin_le)
  MAKE_VIOLATION_INVARIANT(fzn::int_lin_ne)
  MAKE_VIOLATION_INVARIANT(fzn::int_lt)
  MAKE_VIOLATION_INVARIANT(fzn::int_ne)
  MAKE_VIOLATION_INVARIANT(fzn::set_in)
#undef MAKE_VIOLATION_INVARIANT
  throw std::runtime_error(std::string("Failed to create soft constraint: ")
                               .append(constraint.identifier()));
}

}  // namespace atlantis::invariantgraph