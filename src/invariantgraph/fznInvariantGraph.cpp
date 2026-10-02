#include "atlantis/invariantgraph/fznInvariantGraph.hpp"

#include <functional>
#include <unordered_set>
#include <vector>

#include "atlantis/invariantgraph/fzn/array_bool_and.hpp"
#include "atlantis/invariantgraph/fzn/array_bool_element.hpp"
#include "atlantis/invariantgraph/fzn/array_bool_element2d.hpp"
#include "atlantis/invariantgraph/fzn/array_bool_or.hpp"
#include "atlantis/invariantgraph/fzn/array_bool_xor.hpp"
#include "atlantis/invariantgraph/fzn/array_int_element.hpp"
#include "atlantis/invariantgraph/fzn/array_int_element2d.hpp"
#include "atlantis/invariantgraph/fzn/array_int_maximum.hpp"
#include "atlantis/invariantgraph/fzn/array_int_minimum.hpp"
#include "atlantis/invariantgraph/fzn/array_var_bool_element.hpp"
#include "atlantis/invariantgraph/fzn/array_var_bool_element2d.hpp"
#include "atlantis/invariantgraph/fzn/array_var_int_element.hpp"
#include "atlantis/invariantgraph/fzn/array_var_int_element2d.hpp"
#include "atlantis/invariantgraph/fzn/bool2int.hpp"
#include "atlantis/invariantgraph/fzn/bool_and.hpp"
#include "atlantis/invariantgraph/fzn/bool_clause.hpp"
#include "atlantis/invariantgraph/fzn/bool_eq.hpp"
#include "atlantis/invariantgraph/fzn/bool_le.hpp"
#include "atlantis/invariantgraph/fzn/bool_lin_eq.hpp"
#include "atlantis/invariantgraph/fzn/bool_lin_le.hpp"
#include "atlantis/invariantgraph/fzn/bool_lt.hpp"
#include "atlantis/invariantgraph/fzn/bool_not.hpp"
#include "atlantis/invariantgraph/fzn/bool_or.hpp"
#include "atlantis/invariantgraph/fzn/bool_xor.hpp"
#include "atlantis/invariantgraph/fzn/fzn_all_different_int.hpp"
#include "atlantis/invariantgraph/fzn/fzn_all_equal_int.hpp"
#include "atlantis/invariantgraph/fzn/fzn_circuit.hpp"
#include "atlantis/invariantgraph/fzn/fzn_count_eq.hpp"
#include "atlantis/invariantgraph/fzn/fzn_count_geq.hpp"
#include "atlantis/invariantgraph/fzn/fzn_count_gt.hpp"
#include "atlantis/invariantgraph/fzn/fzn_count_leq.hpp"
#include "atlantis/invariantgraph/fzn/fzn_count_lt.hpp"
#include "atlantis/invariantgraph/fzn/fzn_count_neq.hpp"
#include "atlantis/invariantgraph/fzn/fzn_global_cardinality.hpp"
#include "atlantis/invariantgraph/fzn/fzn_global_cardinality_closed.hpp"
#include "atlantis/invariantgraph/fzn/fzn_global_cardinality_low_up.hpp"
#include "atlantis/invariantgraph/fzn/fzn_global_cardinality_low_up_closed.hpp"
#include "atlantis/invariantgraph/fzn/fzn_table_bool.hpp"
#include "atlantis/invariantgraph/fzn/fzn_table_int.hpp"
#include "atlantis/invariantgraph/fzn/int_abs.hpp"
#include "atlantis/invariantgraph/fzn/int_div.hpp"
#include "atlantis/invariantgraph/fzn/int_eq.hpp"
#include "atlantis/invariantgraph/fzn/int_le.hpp"
#include "atlantis/invariantgraph/fzn/int_lin_eq.hpp"
#include "atlantis/invariantgraph/fzn/int_lin_le.hpp"
#include "atlantis/invariantgraph/fzn/int_lin_ne.hpp"
#include "atlantis/invariantgraph/fzn/int_lt.hpp"
#include "atlantis/invariantgraph/fzn/int_max.hpp"
#include "atlantis/invariantgraph/fzn/int_min.hpp"
#include "atlantis/invariantgraph/fzn/int_mod.hpp"
#include "atlantis/invariantgraph/fzn/int_ne.hpp"
#include "atlantis/invariantgraph/fzn/int_plus.hpp"
#include "atlantis/invariantgraph/fzn/int_pow.hpp"
#include "atlantis/invariantgraph/fzn/int_times.hpp"
#include "atlantis/invariantgraph/fzn/set_in.hpp"
#include "atlantis/invariantgraph/varNode.hpp"
#include "atlantis/utils/domains.hpp"
#include "atlantis/utils/fznAst.hpp"
#include "atlantis/utils/fznOutput.hpp"

namespace atlantis::invariantgraph {

DomainType domainType(const std::vector<fznparser::Annotation>& annotations,
                      const DomainType defaultDomainType) {
  for (const auto& annotation : annotations) {
    if (annotation.identifier() == "computed_domain") {
      return DomainType::DOM_NONE;
    }
  }
  return defaultDomainType;
}

DomainType domainType(const fznparser::BoolVar& var) {
  return domainType(var.annotations(), var.isFixed() ? DomainType::DOM_FIXED
                                                     : DomainType::DOM_RANGE);
}

DomainType domainType(const fznparser::IntVar& var) {
  const auto defaultDomainType =
      var.isFixed() ? DomainType::DOM_FIXED
                    : (var.domain().isInterval() ? DomainType::DOM_RANGE
                                                 : DomainType::DOM_DOMAIN);
  return domainType(var.annotations(), defaultDomainType);
}

FznInvariantGraph::FznInvariantGraph(const bool breakDynamicCycles)
    : InvariantGraph(breakDynamicCycles) {}

void FznInvariantGraph::build(const fznparser::Model& model) {
  createNodes(model);

  if (model.hasObjective()) {
    const fznparser::Var& modelObjective = model.objective();
    _objectiveVarNode = varNode(modelObjective.identifier()).ptr();
    if (_objectiveVarNode == nullptr) {
      if (std::holds_alternative<std::shared_ptr<fznparser::BoolVar>>(
              modelObjective)) {
        _objectiveVarNode = retrieveVarNode(
            *std::get<std::shared_ptr<fznparser::BoolVar>>(modelObjective)).ptr();
      } else if (std::holds_alternative<std::shared_ptr<fznparser::IntVar>>(
                     modelObjective)) {
        _objectiveVarNode = retrieveVarNode(
            *std::get<std::shared_ptr<fznparser::IntVar>>(modelObjective)).ptr();
      } else {
        throw FznException("Objective variable is not a BoolVar or IntVar");
      }
    }
    assert(model.solveType().problemType() != fznparser::ProblemType::SATISFY);
    _objectiveDirection =
        model.solveType().problemType() == fznparser::ProblemType::MINIMIZE
            ? ObjectiveDirection::MINIMIZE
            : ObjectiveDirection::MAXIMIZE;
  } else {
    _objectiveDirection = ObjectiveDirection::NONE;
  }
}

VarNode& FznInvariantGraph::retrieveVarNode(
    const fznparser::BoolVar& var) {
  std::shared_ptr<VarNode> vNode{nullptr};
  if (var.isFixed()) {
    vNode = var.identifier().empty()
                ? retrieveBoolVarNode(var.lowerBound()).ptr()
                : retrieveBoolVarNode(var.lowerBound(), var.identifier()).ptr();
  } else if (!var.identifier().empty()) {
    vNode = retrieveBoolVarNode(var.identifier(), domainType(var)).ptr();
  } else {
    throw FznException(
        "Input IntVar must be a parameter or have an identifier");
  }

  vNode->setIsOutputVar(var.isOutput());
  if (var.isOutput() && !var.identifier().empty() &&
      !_outputIdentifiers.contains(var.identifier())) {
    _outputIdentifiers.emplace(var.identifier());
    _outputBoolVars.emplace_back(var.identifier(), vNode);
  }

  return *vNode;
}

VarNode& FznInvariantGraph::retrieveVarNode(
    const std::shared_ptr<const fznparser::BoolVar>& ptr) {
  return retrieveVarNode(*ptr);
}

VarNode& FznInvariantGraph::retrieveVarNode(
    const fznparser::BoolArg& arg) {
  return arg.isParameter() ? retrieveBoolVarNode(arg.parameter())
                           : retrieveVarNode(arg.var());
}

VarNode& FznInvariantGraph::retrieveVarNode(
    const fznparser::IntVar& var) {
  std::shared_ptr<VarNode> vNode{nullptr};
  if (var.isFixed()) {
    vNode = var.identifier().empty()
                ? retrieveIntVarNode(var.lowerBound()).ptr()
                : retrieveIntVarNode(var.lowerBound(), var.identifier()).ptr();
  } else if (!var.identifier().empty()) {
    vNode = retrieveIntVarNode(
        var.domain().isInterval()
            ? std::make_shared<SearchDomain>(var.domain().lowerBound(),
                                             var.domain().upperBound())
            : std::make_shared<SearchDomain>(var.domain().elements()),
        var.identifier(), domainType(var)).ptr();
  } else {
    throw FznException(
        "Input IntVar must be a parameter or have an identifier");
  }

  vNode->setIsOutputVar(var.isOutput());
  if (var.isOutput() && !var.identifier().empty() &&
      !_outputIdentifiers.contains(var.identifier())) {
    _outputIdentifiers.emplace(var.identifier());
    _outputIntVars.emplace_back(var.identifier(), vNode);
  }

  return *vNode;
}

VarNode& FznInvariantGraph::retrieveVarNode(
    const std::shared_ptr<const fznparser::IntVar>& ptr) {
  return retrieveVarNode(*ptr);
}

VarNode& FznInvariantGraph::retrieveVarNode(
    const fznparser::IntArg& arg) {
  return arg.isParameter() ? retrieveIntVarNode(arg.parameter())
                           : retrieveVarNode(arg.var());
}

std::vector<std::shared_ptr<VarNode>> FznInvariantGraph::retrieveVarNodes(
    const std::shared_ptr<fznparser::BoolVarArray>& array) {
  std::vector<std::shared_ptr<VarNode>> varNodes;
  varNodes.reserve(array->size());

  for (size_t i = 0; i < array->size(); ++i) {
    varNodes.emplace_back(
        std::holds_alternative<bool>(array->at(i))
            ? retrieveBoolVarNode(std::get<bool>(array->at(i))).ptr()
            : retrieveVarNode(
                  std::get<std::shared_ptr<const fznparser::BoolVar>>(
                      array->at(i))).ptr());
  }

  for (const auto& vNode : varNodes) {
    vNode->setIsOutputVar(array->isOutput());
  }
  if (array->isOutput() && !array->identifier().empty() &&
      !_outputIdentifiers.contains(array->identifier())) {
    _outputIdentifiers.emplace(array->identifier());
    _outputBoolVarArrays.emplace_back(array->identifier(),
                                      array->outputIndexSetSizes(), varNodes);
  }

  return varNodes;
}

std::vector<std::shared_ptr<VarNode>> FznInvariantGraph::retrieveVarNodes(
    const std::shared_ptr<fznparser::IntVarArray>& array) {
  std::vector<std::shared_ptr<VarNode>> varNodes;
  varNodes.reserve(array->size());

  for (size_t i = 0; i < array->size(); ++i) {
    varNodes.emplace_back(
        std::holds_alternative<Int>(array->at(i))
            ? retrieveIntVarNode(std::get<Int>(array->at(i))).ptr()
            : retrieveVarNode(
                  std::get<std::shared_ptr<const fznparser::IntVar>>(
                      array->at(i))).ptr());
  }

  for (const auto& vNode : varNodes) {
    vNode->setIsOutputVar(array->isOutput());
  }
  if (array->isOutput() && !array->identifier().empty() &&
      !_outputIdentifiers.contains(array->identifier())) {
    _outputIdentifiers.emplace(array->identifier());
    _outputIntVarArrays.emplace_back(array->identifier(),
                                     array->outputIndexSetSizes(), varNodes);
  }

  return varNodes;
}

std::vector<FznOutputVar> FznInvariantGraph::outputBoolVars() const noexcept {
  std::vector<FznOutputVar> outputVars;
  outputVars.reserve(_outputBoolVars.size());
  for (const auto& [identifier, vNode] : _outputBoolVars) {
    if (vNode->isFixed() ||
        (vNode->staticInputTo().empty() && vNode->dynamicInputTo().empty() &&
         vNode->definingNodes().empty())) {
      outputVars.emplace_back(identifier, vNode->lowerBound());
    } else {
      outputVars.emplace_back(identifier, vNode);
    }
  }
  return outputVars;
}

std::vector<FznOutputVar> FznInvariantGraph::outputIntVars() const noexcept {
  std::vector<FznOutputVar> outputVars;
  outputVars.reserve(_outputIntVars.size());
  for (const auto& [identifier, vNode] : _outputIntVars) {
    if (vNode->isFixed() ||
        (vNode->staticInputTo().empty() && vNode->dynamicInputTo().empty() &&
         vNode->definingNodes().empty())) {
      outputVars.emplace_back(identifier, vNode->lowerBound());
    } else {
      outputVars.emplace_back(identifier, vNode);
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
    fznArray.vars.reserve(outputArray.varNodes.size());
    for (const auto& vNode : outputArray.varNodes) {
      if (vNode->isFixed() ||
          (vNode->staticInputTo().empty() && vNode->dynamicInputTo().empty() &&
           vNode->definingNodes().empty())) {
        fznArray.vars.emplace_back(vNode->lowerBound());
      } else {
        fznArray.vars.emplace_back(vNode);
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
    fznArray.vars.reserve(outputArray.varNodes.size());
    for (const auto& vNode : outputArray.varNodes) {
      if (vNode->isFixed() ||
          (vNode->staticInputTo().empty() && vNode->dynamicInputTo().empty() &&
           vNode->definingNodes().empty())) {
        fznArray.vars.emplace_back(vNode->lowerBound());
      } else {
        fznArray.vars.emplace_back(vNode);
      }
    }
  }
  return outputVarArrays;
}
FznOutput FznInvariantGraph::generateFznOutput() const {
  return {outputBoolVars(), outputIntVars(), outputBoolVarArrays(),
          outputIntVarArrays()};
}

void FznInvariantGraph::createNodes(const fznparser::Model& model) {
  std::unordered_set<std::string> definedVars;
  std::vector<bool> constraintIsProcessed(model.constraints().size(), false);

  const std::vector<std::function<bool(const fznparser::Constraint&)>>
      invariantNodeCreators{
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

  assert(std::ranges::all_of(constraintIsProcessed.begin(),
                             constraintIsProcessed.end(),
                             [](const bool b) { return b; }));

  if (model.hasObjective()) {
    if (std::holds_alternative<std::shared_ptr<fznparser::BoolVar>>(
            model.objective())) {
      retrieveVarNode(
          *std::get<std::shared_ptr<fznparser::BoolVar>>(model.objective()));
    } else if (std::holds_alternative<std::shared_ptr<fznparser::IntVar>>(
                   model.objective())) {
      retrieveVarNode(
          *std::get<std::shared_ptr<fznparser::IntVar>>(model.objective()));
    } else {
      throw FznException("Objective variable is not a bool or int var");
    }
  }

  for (const auto& [identifier, var] : model.vars()) {
    assert(!identifier.empty());
    if (std::holds_alternative<std::shared_ptr<fznparser::IntVar>>(var)) {
      const auto& intVar = std::get<std::shared_ptr<fznparser::IntVar>>(var);
      if (intVar->isFixed()) {
        continue;
      }
      if (!containsVarNode(identifier)) {
        retrieveVarNode(*intVar);
      }
      assert(varNode(identifier).isIntVar());
    } else if (std::holds_alternative<std::shared_ptr<fznparser::BoolVar>>(
                   var)) {
      const auto& boolVar = std::get<std::shared_ptr<fznparser::BoolVar>>(var);
      if (boolVar->isFixed()) {
        continue;
      }
      if (!containsVarNode(identifier)) {
        retrieveVarNode(*boolVar);
      }
      assert(!varNode(identifier).isIntVar());
    } else if (std::holds_alternative<std::shared_ptr<fznparser::IntVarArray>>(
                   var)) {
      const auto& intVarArray =
          std::get<std::shared_ptr<fznparser::IntVarArray>>(var);
      if (intVarArray->isOutput() && !_outputIdentifiers.contains(identifier)) {
        retrieveVarNodes(intVarArray);
      }
    } else if (std::holds_alternative<std::shared_ptr<fznparser::BoolVarArray>>(
                   var)) {
      const auto& boolVarArray =
          std::get<std::shared_ptr<fznparser::BoolVarArray>>(var);
      if (boolVarArray->isOutput() &&
          !_outputIdentifiers.contains(identifier)) {
        retrieveVarNodes(boolVarArray);
      }
    }
  }
}

bool FznInvariantGraph::makeInvariantNode(
    const fznparser::Constraint& constraint) {
#define MAKE_INVARIANT(fznConstraintName)     \
  if (fznConstraintName(*this, constraint)) { \
    return true;                              \
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
  MAKE_INVARIANT(fzn::fzn_circuit)
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
  MAKE_VIOLATION_INVARIANT(fzn::fzn_table_int)
  MAKE_VIOLATION_INVARIANT(fzn::fzn_table_bool)
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
