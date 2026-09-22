#include "./fznTestBase.hpp"

#include <rapidcheck/gen/Numeric.h>

#include <ranges>

#include "atlantis/search/neighborhoods/randomNeighborhood.hpp"
#include "atlantis/search/searchVariable.hpp"
#include "atlantis/utils/domains.hpp"

namespace atlantis::testing {
using namespace fznparser;
using namespace atlantis::invariantgraph;

std::string to_string(const bool v) { return v ? "true" : "false"; }

void FznTestBase::SetUp() {
  _model = std::make_shared<Model>();
  _invariantGraph = std::make_shared<FznInvariantGraph>(true);
  _solver = nullptr;
  _solverMapping = nullptr;

  // Keep the non-RapidCheck move generation deterministic so failing property
  // cases can actually be reproduced from RC_PARAMS.
  gen = std::mt19937(0x5eed1234u);
  binaryDist = std::uniform_int_distribution<unsigned char>(0, 1);
}

void FznTestBase::generateConstraint() {
  if (annotations.empty()) {
    _model->addConstraint(
        Constraint{constraintIdentifier, std::vector<Arg>{args}});
  } else {
    _model->addConstraint(Constraint{constraintIdentifier,
                                     std::vector<Arg>{args},
                                     std::vector<Annotation>{annotations}});
  }
  _invariantGraph->open();
  _invariantGraph->build(*_model);
}

void FznTestBase::closeInvariantGraph() {
  _invariantGraph->open();
  _invariantGraph->close();
  _solver = std::make_shared<propagation::Solver>();
  _solverMapping =
      std::make_shared<SolverMapping>(_invariantGraph->construct(*_solver));
}

VarNode& FznTestBase::varNode(const std::string& identifier) {
  RC_LOG() << "varNode(\"" << identifier << "\")" << std::endl;
  RC_ASSERT(_invariantGraph->containsVarNode(identifier));
  return _invariantGraph->varNode(identifier);
}

const VarNode& FznTestBase::varNodeConst(const std::string& identifier) const {
  RC_LOG() << "varNodeConst(\"" << identifier << "\")" << std::endl;
  RC_ASSERT(_invariantGraph->containsVarNode(identifier));
  return _invariantGraph->varNodeConst(identifier);
}

VarNodeId FznTestBase::varNodeId(const std::string& identifier) const {
  if (_invariantGraph->containsVarNode(identifier)) {
    return _invariantGraph->varNodeId(identifier);
  }
  return NULL_NODE_ID;
}

propagation::VarViewId FznTestBase::varId(const std::string& identifier) const {
  return _invariantGraph->containsVarNode(identifier) &&
                 _solverMapping != nullptr
             ? _solverMapping->solverId(_invariantGraph->varNodeId(identifier))
             : propagation::VAR_VIEW_NULL_ID;
}

propagation::VarViewId FznTestBase::varId(const VarNodeId vId) const {
  return _solverMapping->solverId(vId);
}

void FznTestBase::setValue(const std::string& identifier, const Int val) const {
  _solver->setValue(varId(identifier), val);
}
Int FznTestBase::currentValue(const std::string& identifier) const {
  return _solver->currentValue(varId(identifier));
}
Int FznTestBase::lowerBound(const std::string& identifier) const {
  if (_invariantGraph->containsVarNode(identifier)) {
    return varNodeConst(identifier).lowerBound();
  }
  if (intPars.contains(identifier)) {
    return intPars.at(identifier);
  }
  if (boolPars.contains(identifier)) {
    return boolPars.at(identifier) ? 0 : 1;
  }
  if (_model->hasVar(identifier)) {
    if (std::holds_alternative<std::shared_ptr<BoolVar>>(
            _model->var(identifier))) {
      return std::get<std::shared_ptr<BoolVar>>(_model->var(identifier))
                     ->contains(true)
                 ? 0
                 : 1;
    }
    if (std::holds_alternative<std::shared_ptr<IntVar>>(
            _model->var(identifier))) {
      return std::get<std::shared_ptr<IntVar>>(_model->var(identifier))
          ->lowerBound();
    }
  }
  RC_LOG() << "unknown var \"" << identifier << "\"" << std::endl;
  RC_FAIL();
}
Int FznTestBase::upperBound(const std::string& identifier) const {
  if (_invariantGraph->containsVarNode(identifier)) {
    return varNodeConst(identifier).upperBound();
  }
  if (intPars.contains(identifier)) {
    return intPars.at(identifier);
  }
  if (boolPars.contains(identifier)) {
    return boolPars.at(identifier) ? 0 : 1;
  }
  if (_model->hasVar(identifier)) {
    if (std::holds_alternative<std::shared_ptr<BoolVar>>(
            _model->var(identifier))) {
      return std::get<std::shared_ptr<BoolVar>>(_model->var(identifier))
                     ->contains(false)
                 ? 1
                 : 0;
    }
    if (std::holds_alternative<std::shared_ptr<IntVar>>(
            _model->var(identifier))) {
      return std::get<std::shared_ptr<IntVar>>(_model->var(identifier))
          ->upperBound();
    }
  }
  RC_LOG() << "unknown var \"" << identifier << "\"" << std::endl;
  RC_FAIL();
}
bool FznTestBase::isFixed(const std::string& identifier) const {
  if (_invariantGraph->containsVarNode(identifier)) {
    return varNodeConst(identifier).isFixed();
  }
  if (boolPars.contains(identifier) || intPars.contains(identifier)) {
    return true;
  }
  RC_LOG() << "unknown var \"" << identifier << "\"" << std::endl;
  RC_FAIL();
}
bool FznTestBase::boolVal(const std::string& identifier,
                          const bool committedValue) const {
  std::optional<bool> ret{};
  if (_invariantGraph->containsVarNode(identifier)) {
    const auto& vNode = varNodeConst(identifier);
    RC_ASSERT(!vNode.isIntVar());
    if (_solverMapping != nullptr &&
        _solverMapping->solverId(vNode.varNodeId()) != propagation::NULL_ID) {
      ret = (committedValue ? _solver->committedValue(
                                  _solverMapping->solverId(vNode.varNodeId()))
                            : _solver->currentValue(_solverMapping->solverId(
                                  vNode.varNodeId()))) == 0;
    } else if (vNode.constDomain()->size() > 0) {
      RC_ASSERT(vNode.isFixed() || !vNode.isOutputVar());
      ret = vNode.inDomain(bool{true});
    }
  } else if (boolPars.contains(identifier)) {
    ret = boolPars.at(identifier);
  }
  if (ret.has_value()) {
    RC_LOG() << "boolVal(\"" << identifier
             << "\", committedValue=" << to_string(committedValue)
             << ") = " << to_string(*ret) << std::endl;
    return *ret;
  }
  RC_LOG() << "boolVal(\"" << identifier
           << "\", committedValue=" << to_string(committedValue)
           << ") unhandled argument type" << std::endl;
  RC_FAIL();
}
bool FznTestBase::inDomain(const std::string& identifier,
                           const bool val) const {
  RC_LOG() << "inDomain(\"" << identifier << "\", " << to_string(val) << ')'
           << std::endl;
  if (_invariantGraph->containsVarNode(identifier)) {
    const auto& vNode = varNodeConst(identifier);
    RC_ASSERT(!vNode.isIntVar());
    return vNode.inDomain(val);
  }
  if (boolPars.contains(identifier)) {
    return boolPars.at(identifier) == val;
  }
  RC_LOG() << "unhandled argument type" << std::endl;
  RC_FAIL();
}
bool FznTestBase::isFixedTo(const std::string& identifier,
                            const bool val) const {
  if (isFixed(identifier)) {
    return boolVal(identifier, val) == val;
  }
  return false;
}
bool FznTestBase::isFixedTo(const std::string& identifier,
                            const Int val) const {
  if (isFixed(identifier)) {
    return intVal(identifier, val) == val;
  }
  return false;
}

Int FznTestBase::intVal(const std::string& identifier,
                        const bool committedValue) const {
  std::optional<Int> ret{};
  if (_invariantGraph->containsVarNode(identifier)) {
    const auto& vNode = varNodeConst(identifier);
    RC_ASSERT(vNode.isIntVar());
    if (_solverMapping != nullptr &&
        _solverMapping->solverId(vNode.varNodeId()) != propagation::NULL_ID) {
      ret = committedValue ? _solver->committedValue(
                                 _solverMapping->solverId(vNode.varNodeId()))
                           : _solver->currentValue(
                                 _solverMapping->solverId(vNode.varNodeId()));
    } else {
      RC_ASSERT(vNode.isFixed() || !vNode.isOutputVar());
      ret = vNode.lowerBound();
    }
  } else if (intPars.contains(identifier)) {
    ret = intPars.at(identifier);
  }
  if (ret.has_value()) {
    RC_LOG() << "intVal(\"" << identifier
             << "\", committedValue=" << to_string(committedValue)
             << ") = " << *ret << std::endl;
    return *ret;
  }
  RC_LOG() << "intVal(\"" << identifier
           << "\", committedValue=" << to_string(committedValue)
           << ") unhandled argument type" << std::endl;
  RC_FAIL();
}

bool FznTestBase::inDomain(const std::string& identifier, const Int val) const {
  RC_LOG() << "inDomain(\"" << identifier << "\", " << val << ')' << std::endl;
  if (_invariantGraph->containsVarNode(identifier)) {
    const auto& vNode = varNodeConst(identifier);
    RC_ASSERT(vNode.isIntVar());
    return vNode.inDomain(val);
  }
  if (intPars.contains(identifier)) {
    return intPars.at(identifier) == val;
  }
  RC_LOG() << "unhandled argument type" << std::endl;
  RC_FAIL();
}

const std::vector<Int>& FznTestBase::intSetVal(
    const std::string& identifier) const {
  RC_LOG() << "intSetVal(\"" << identifier << ')' << std::endl;
  if (intSetPars.contains(identifier)) {
    return intSetPars.at(identifier);
  }
  RC_LOG() << "unhandled argument type" << std::endl;
  RC_FAIL();
}

propagation::VarViewId FznTestBase::totalViolationVarId() const {
  return _solverMapping == nullptr ? propagation::VAR_VIEW_NULL_ID
                                   : _solverMapping->totalViolationId();
}
Int FznTestBase::violation(const bool committedValue) const {
  if (totalViolationVarId() == propagation::NULL_ID) {
    return 0;
  }
  return committedValue ? _solver->committedValue(totalViolationVarId())
                        : _solver->currentValue(totalViolationVarId());
}
std::shared_ptr<IntVar> FznTestBase::genIntVar(const Int lb, const Int ub,
                                               const std::string& identifier) {
  if (lb == ub) {
    addIntPar(identifier, lb);
  }
  return std::get<std::shared_ptr<IntVar>>(
      _model->addVar(std::make_shared<IntVar>(lb, ub, identifier)));
}

std::shared_ptr<IntVar> FznTestBase::genIntVar(const std::vector<Int>& dom,
                                               const std::string& identifier) {
  RC_ASSERT(!dom.empty());
  if (dom.size() == 1) {
    addIntPar(identifier, dom.front());
  }
  return std::get<std::shared_ptr<IntVar>>(
      _model->addVar(std::make_shared<IntVar>(std::vector{dom}, identifier)));
}

std::shared_ptr<IntVar> FznTestBase::genIntVar(const std::string& identifier) {
  return genIntVar(defaultLb, defaultUb, identifier);
}
std::shared_ptr<IntVar> FznTestBase::genIntVar(const IntArgState state,
                                               const std::string& identifier) {
  return genIntVar(state, defaultLb, defaultUb, identifier);
}
std::shared_ptr<IntVar> FznTestBase::genIntVar(const IntArgState state,
                                               const Int lb, const Int ub,
                                               const std::string& identifier) {
  switch (state) {
    case IntArgState::FIXED: {
      const Int val = lb == ub ? lb : *rc::gen::inRange<Int>(lb, ub + 1);
      return genIntVar(val, val, identifier);
    }
    case IntArgState::VAR: {
      return genIntVar(lb, ub, identifier);
    }
    default:
      RC_LOG() << "unhandled IntArgState" << std::endl;
      RC_FAIL();
  }
}

void FznTestBase::addBoolPar(const std::string& identifier, const bool val) {
  RC_LOG() << "addBoolPar(\"" << identifier << "\", " << to_string(val) << ')'
           << std::endl;
  RC_ASSERT(!identifier.empty());
  RC_ASSERT(!boolPars.contains(identifier));
  RC_ASSERT(!intPars.contains(identifier));
  RC_ASSERT(!intSetPars.contains(identifier));
  boolPars.emplace(identifier, val);
}

void FznTestBase::addIntPar(const std::string& identifier, const Int val) {
  RC_LOG() << "addIntPar(\"" << identifier << "\", " << val << ')' << std::endl;
  RC_ASSERT(!identifier.empty());
  RC_ASSERT(!boolPars.contains(identifier));
  RC_ASSERT(!intPars.contains(identifier));
  RC_ASSERT(!intSetPars.contains(identifier));
  intPars.emplace(identifier, val);
}

void FznTestBase::addIntSetPar(const std::string& identifier,
                               std::vector<Int>&& vals) {
  RC_LOG() << "addIntSetPar(\"" << identifier << "\", {";
  for (size_t i = 0; i < vals.size(); ++i) {
    RC_LOG() << (i == 0 ? "" : ", ") << vals.at(i);
  }
  RC_LOG() << "})" << std::endl;
  RC_ASSERT(!identifier.empty());
  RC_ASSERT(!boolPars.contains(identifier));
  RC_ASSERT(!intPars.contains(identifier));
  RC_ASSERT(!intSetPars.contains(identifier));
  intSetPars.emplace(identifier, std::move(vals));
}

IntArg FznTestBase::_addIntArg(const IntArgState state, const Int val,
                               const std::string& identifier) {
  RC_ASSERT(state != IntArgState::VAR);
  switch (state) {
    case IntArgState::PAR: {
      addIntPar(identifier, val);
      auto arg = IntArg{val};
      args.emplace_back(arg);
      return arg;
    }
    case IntArgState::FIXED: {
      auto var = genIntVar(val, val, identifier);
      args.emplace_back(var);
      return var;
    }
    default:
      RC_LOG() << "Invalid IntArgState" << std::endl;
      RC_FAIL();
  }
}

IntArg FznTestBase::addIntArg(const IntArgState state, const Int val,
                              const std::string& identifier) {
  return _addIntArg(state, val, identifier);
}

IntArg FznTestBase::_addIntArg(const IntArgState state, const Int lb,
                               const Int ub, const std::string& identifier) {
  switch (state) {
    case IntArgState::PAR: {
      const Int val = lb == ub ? lb : *rc::gen::inRange<Int>(lb, ub + 1);
      addIntPar(identifier, val);
      auto arg = IntArg{val};
      args.emplace_back(arg);
      return arg;
    }
    case IntArgState::FIXED: {
      const Int val = lb == ub ? lb : *rc::gen::inRange<Int>(lb, ub + 1);
      auto var = genIntVar(val, val, identifier);
      args.emplace_back(var);
      return var;
    }
    case IntArgState::VAR: {
      auto var = genIntVar(lb, ub, identifier);
      args.emplace_back(var);
      return var;
    }
    default:
      RC_LOG() << "Invalid IntArgState" << std::endl;
      RC_FAIL();
  }
}

IntArg FznTestBase::addIntArg(const IntArgState state, const Int lb,
                              const Int ub, const std::string& identifier) {
  return _addIntArg(state, lb, ub, identifier);
}

IntArg FznTestBase::_addIntArg(const IntArgState state,
                               const std::vector<Int>& dom,
                               const std::string& identifier) {
  RC_ASSERT(!dom.empty());
  switch (state) {
    case IntArgState::PAR: {
      const Int val = dom.size() == 1 ? dom.front() : *rc::gen::elementOf(dom);
      addIntPar(identifier, val);
      auto arg = IntArg{val};
      args.emplace_back(arg);
      return arg;
    }
    case IntArgState::FIXED: {
      const Int val = dom.size() == 1 ? dom.front() : *rc::gen::elementOf(dom);
      auto var = genIntVar(val, val, identifier);
      args.emplace_back(var);
      return var;
    }
    case IntArgState::VAR: {
      RC_ASSERT(dom.size() > size_t{1});
      auto var = genIntVar(dom, identifier);
      args.emplace_back(var);
      return var;
    }
    default:
      RC_LOG() << "Invalid IntArgState" << std::endl;
      RC_FAIL();
  }
}

IntArg FznTestBase::addIntArg(const IntArgState state,
                              const std::vector<Int>& dom,
                              const std::string& identifier) {
  return _addIntArg(state, dom, identifier);
}

IntArg FznTestBase::_addIntArg(const IntArgState state,
                               const std::string& identifier) {
  return _addIntArg(state, defaultLb, defaultUb, identifier);
}

IntArg FznTestBase::addIntArg(const IntArgState state,
                              const std::string& identifier) {
  return _addIntArg(state, identifier);
}

IntArg FznTestBase::_addIntArg(const Int lb, const Int ub,
                               const std::string& identifier) {
  return _addIntArg(
      lb == ub ? *rc::gen::element(IntArgState::PAR, IntArgState::FIXED)
               : *rc::gen::arbitrary<IntArgState>(),
      lb, ub, identifier);
}

IntArg FznTestBase::addIntArg(const Int lb, const Int ub,
                              const std::string& identifier) {
  return _addIntArg(lb, ub, identifier);
}

IntArg FznTestBase::addIntArg(const std::string& identifier) {
  return _addIntArg(defaultLb, defaultUb, identifier);
}

std::vector<Int> FznTestBase::addIntParArray(const std::vector<Int>& pars,
                                             const std::string& identifier) {
  auto parArray = std::get<std::shared_ptr<IntVarArray>>(
      _model->addVar(std::make_shared<IntVarArray>(identifier)));
  for (Int par : pars) {
    parArray->append(par);
  }
  args.emplace_back(parArray);
  return pars;
}

std::vector<Int> FznTestBase::addIntParArray(const size_t arraySize,
                                             const Int lb, const Int ub,
                                             const std::string& identifier) {
  const std::vector<Int> pars = *rc::gen::container<std::vector<Int>>(
      arraySize, rc::gen::inRange<Int>(lb, ub));
  return addIntParArray(pars, identifier);
}
std::shared_ptr<IntVarArray> FznTestBase::genIntVarArray(
    const size_t arraySize, const Int lb, const Int ub,
    const std::string& identifier, const std::string& varPrefix) {
  auto vars = std::get<std::shared_ptr<IntVarArray>>(
      _model->addVar(std::make_shared<IntVarArray>(identifier)));
  const std::vector<IntArgState> argStates =
      *rc::gen::container<std::vector<IntArgState>>(
          arraySize, rc::gen::arbitrary<IntArgState>());
  for (size_t i = 0; i < arraySize; ++i) {
    switch (argStates.at(i)) {
      case IntArgState::PAR:
        vars->append(*rc::gen::inRange<Int>(lb, ub + 1));
        break;
      case IntArgState::FIXED: {
        const Int val = *rc::gen::inRange<Int>(lb, ub + 1);
        vars->append(genIntVar(val, val, varPrefix + std::to_string(i)));
        break;
      }
      case IntArgState::VAR:
        vars->append(genIntVar(lb, ub, varPrefix + std::to_string(i)));
        break;
      default:
        RC_LOG() << "unknown IntArgState" << std::endl;
        RC_FAIL();
    }
  }
  return vars;
}
std::shared_ptr<IntVarArray> FznTestBase::genIntVarArray(
    const size_t arraySize, const std::string& identifier,
    const std::string& varPrefix) {
  return genIntVarArray(arraySize, defaultLb, defaultUb, identifier, varPrefix);
}

std::vector<Int> FznTestBase::addIntParArray(const size_t arraySize,
                                             const std::string& identifier) {
  return addIntParArray(arraySize, defaultLb, defaultUb, identifier);
}

std::shared_ptr<BoolVar> FznTestBase::genBoolVar(
    const BoolArgState state, const std::string& identifier) {
  switch (state) {
    case BoolArgState::FIXED_FALSE:
    case BoolArgState::FIXED_TRUE: {
      auto fixedVar = std::make_shared<BoolVar>(
          state == BoolArgState::FIXED_TRUE, identifier);
      addBoolPar(identifier, state == BoolArgState::FIXED_TRUE);
      _model->addVar(fixedVar);
      return fixedVar;
    }
    case BoolArgState::VAR: {
      auto var = std::make_shared<BoolVar>(identifier);
      _model->addVar(var);
      return var;
    }
    default:
      RC_LOG() << "invalid BoolArgState" << std::endl;
      RC_FAIL();
  }
}

std::vector<Int> FznTestBase::genDomain(const size_t size) const {
  RC_ASSERT(size < static_cast<size_t>(defaultUb - defaultLb + 2));
  return *rc::gen::unique<std::vector<Int>>(
      size, rc::gen::inRange<Int>(defaultLb, defaultUb + 1));
}

std::vector<Int> FznTestBase::genDomain(const IntArgState state) const {
  const size_t size =
      state != IntArgState::VAR
          ? 1
          : *rc::gen::inRange<size_t>(2, defaultUb - defaultLb + 2);
  return genDomain(size);
}

std::vector<Int> FznTestBase::genDomain() const {
  return genDomain(*rc::gen::arbitrary<IntArgState>());
}

BoolArg FznTestBase::_addBoolArg(const BoolArgState state,
                                 const std::string& identifier) {
  switch (state) {
    case BoolArgState::PAR_FALSE:
    case BoolArgState::PAR_TRUE: {
      BoolArg par{state == BoolArgState::PAR_TRUE};
      addBoolPar(identifier, state == BoolArgState::PAR_TRUE);
      args.emplace_back(par);
      return par;
    }
    default: {
      auto var = genBoolVar(state, identifier);
      args.emplace_back(var);
      return var;
    }
  }
}

BoolArg FznTestBase::addBoolArg(const BoolArgState state,
                                const std::string& identifier) {
  return _addBoolArg(state, identifier);
}

BoolArg FznTestBase::addBoolArg(const std::string& identifier) {
  return _addBoolArg(*rc::gen::arbitrary<BoolArgState>(), identifier);
}

std::shared_ptr<BoolVarArray> FznTestBase::addBoolParArray(
    const std::vector<bool>& pars, const std::string& identifier) {
  auto parArray = std::get<std::shared_ptr<BoolVarArray>>(
      _model->addVar(std::make_shared<BoolVarArray>(identifier)));
  for (size_t i = 0; i < pars.size(); ++i) {
    parArray->append(pars.at(i));
  }
  args.emplace_back(parArray);
  return parArray;
}

std::shared_ptr<BoolVarArray> FznTestBase::addBoolParArray(
    const size_t arraySize, const std::string& identifier) {
  const std::vector<bool> pars = *rc::gen::container<std::vector<bool>>(
      arraySize, rc::gen::arbitrary<bool>());
  return addBoolParArray(pars, identifier);
}

std::shared_ptr<BoolVarArray> FznTestBase::_addBoolVarArray(
    const size_t arraySize, const std::string& identifier,
    const std::string& varPrefix) {
  const std::vector<BoolArgState> argStates =
      *rc::gen::container<std::vector<BoolArgState>>(
          arraySize, rc::gen::arbitrary<BoolArgState>());
  auto vars = std::get<std::shared_ptr<BoolVarArray>>(
      _model->addVar(std::make_shared<BoolVarArray>(identifier)));
  for (size_t i = 0; i < arraySize; ++i) {
    const std::string varIdentifier = varPrefix + std::to_string(i);
    switch (argStates.at(i)) {
      case BoolArgState::PAR_FALSE:
      case BoolArgState::PAR_TRUE:
        vars->append(argStates.at(i) == BoolArgState::PAR_TRUE);
        addBoolPar(varIdentifier, argStates.at(i) == BoolArgState::PAR_TRUE);
        break;
      default:
        vars->append(genBoolVar(argStates.at(i), varIdentifier));
        break;
    }
  }
  args.emplace_back(vars);
  return vars;
}

std::shared_ptr<BoolVarArray> FznTestBase::addBoolVarArray(
    const size_t arraySize, const std::string& identifier,
    const std::string& varPrefix) {
  return _addBoolVarArray(arraySize, identifier, varPrefix);
}

std::shared_ptr<BoolVarArray> FznTestBase::_addBoolVarArray(
    const std::vector<BoolArgState>& argStates,
    const std::vector<std::string>& identifiers,
    const std::string& identifier) {
  RC_ASSERT(argStates.size() == identifiers.size());
  auto vars = std::get<std::shared_ptr<BoolVarArray>>(
      _model->addVar(std::make_shared<BoolVarArray>(identifier)));
  for (size_t i = 0; i < identifiers.size(); ++i) {
    switch (argStates.at(i)) {
      case BoolArgState::PAR_FALSE:
      case BoolArgState::PAR_TRUE:
        vars->append(argStates.at(i) == BoolArgState::PAR_TRUE);
        addBoolPar(identifiers.at(i),
                   argStates.at(i) == BoolArgState::PAR_TRUE);
        break;
      default:
        vars->append(genBoolVar(argStates.at(i), identifiers.at(i)));
        break;
    }
  }
  args.emplace_back(vars);
  return vars;
}

std::shared_ptr<BoolVarArray> FznTestBase::addBoolVarArray(
    const std::vector<BoolArgState>& argStates,
    const std::vector<std::string>& identifiers,
    const std::string& identifier) {
  return _addBoolVarArray(argStates, identifiers, identifier);
}

std::shared_ptr<BoolVarArray> FznTestBase::addBoolVarArray(
    const std::vector<std::string>& identifiers,
    const std::string& identifier) {
  const std::vector<BoolArgState> argStates =
      *rc::gen::container<std::vector<BoolArgState>>(
          identifiers.size(), rc::gen::arbitrary<BoolArgState>());
  return _addBoolVarArray(argStates, identifiers, identifier);
}

std::shared_ptr<IntVarArray> FznTestBase::_addIntVarArray(
    const std::vector<IntArgState>& argStates,
    const std::vector<std::pair<Int, Int>>& domains,
    const std::vector<std::string>& identifiers,
    const std::string& identifier) {
  RC_ASSERT(argStates.size() == identifiers.size());
  RC_ASSERT(domains.size() == identifiers.size());
  auto vars = std::get<std::shared_ptr<IntVarArray>>(
      _model->addVar(std::make_shared<IntVarArray>(identifier)));
  const size_t numFixed = std::ranges::count_if(
      argStates, [](const IntArgState s) { return s != IntArgState::VAR; });
  const std::vector<Int> fixedVals =
      numFixed == 0
          ? std::vector<Int>{}
          : *rc::gen::container<std::vector<Int>>(
                numFixed, rc::gen::inRange<Int>(defaultLb, defaultUb + 1));
  size_t valIndex = 0;
  for (size_t i = 0; i < identifiers.size(); ++i) {
    const auto [lb, ub] = domains.at(i);
    RC_ASSERT(lb <= ub);
    switch (argStates.at(i)) {
      case IntArgState::PAR: {
        const Int val = fixedVals.at(valIndex++);
        vars->append(val);
        addIntPar(identifiers.at(i), val);
        break;
      }
      case IntArgState::FIXED: {
        const Int val = fixedVals.at(valIndex++);
        vars->append(genIntVar(argStates.at(i), val, val, identifiers.at(i)));
        addIntPar(identifiers.at(i), val);
        break;
      }
      default:
        vars->append(genIntVar(argStates.at(i), lb, ub, identifiers.at(i)));
        break;
    }
  }
  args.emplace_back(vars);
  return vars;
}

std::shared_ptr<IntVarArray> FznTestBase::addIntVarArray(
    const std::vector<IntArgState>& argStates,
    const std::vector<std::pair<Int, Int>>& domains,
    const std::vector<std::string>& identifiers,
    const std::string& identifier) {
  return _addIntVarArray(argStates, domains, identifiers, identifier);
}

std::shared_ptr<IntVarArray> FznTestBase::_addIntVarArray(
    const std::vector<IntArgState>& argStates,
    const std::vector<Int>& fixedVals,
    const std::vector<std::string>& identifiers,
    const std::string& identifier) {
  RC_ASSERT(argStates.size() == identifiers.size());
  const size_t numFixedStates = std::ranges::count_if(
      argStates, [](const IntArgState s) { return s != IntArgState::VAR; });
  RC_ASSERT(fixedVals.size() == numFixedStates);
  auto vars = std::get<std::shared_ptr<IntVarArray>>(
      _model->addVar(std::make_shared<IntVarArray>(identifier)));
  size_t valIndex = 0;
  for (size_t i = 0; i < identifiers.size(); ++i) {
    switch (argStates.at(i)) {
      case IntArgState::VAR:
        vars->append(genIntVar(argStates.at(i), defaultLb, defaultUb,
                               identifiers.at(i)));
        break;
      case IntArgState::PAR: {
        const Int val = fixedVals.at(valIndex++);
        vars->append(val);
        addIntPar(identifiers.at(i), val);
        break;
      }
      case IntArgState::FIXED: {
        const Int val = fixedVals.at(valIndex++);
        vars->append(genIntVar(argStates.at(i), val, val, identifiers.at(i)));
        break;
      }
      default:
        break;
    }
  }
  args.emplace_back(vars);
  return vars;
}

std::shared_ptr<IntVarArray> FznTestBase::addIntVarArray(
    const std::vector<IntArgState>& argStates,
    const std::vector<Int>& fixedVals,
    const std::vector<std::string>& identifiers,
    const std::string& identifier) {
  return _addIntVarArray(argStates, fixedVals, identifiers, identifier);
}

std::shared_ptr<IntVarArray> FznTestBase::_addIntVarArray(
    const std::vector<IntArgState>& argStates,
    const std::vector<std::string>& identifiers,
    const std::string& identifier) {
  RC_ASSERT(argStates.size() == identifiers.size());
  const size_t numFixed = std::ranges::count_if(
      argStates, [](const IntArgState s) { return s != IntArgState::VAR; });
  const std::vector<Int> fixedVals =
      numFixed == 0
          ? std::vector<Int>{}
          : *rc::gen::container<std::vector<Int>>(
                numFixed, rc::gen::inRange<Int>(defaultLb, defaultUb + 1));
  return _addIntVarArray(argStates, fixedVals, identifiers, identifier);
}

std::shared_ptr<IntVarArray> FznTestBase::addIntVarArray(
    const std::vector<IntArgState>& argStates,
    const std::vector<std::string>& identifiers,
    const std::string& identifier) {
  return _addIntVarArray(argStates, identifiers, identifier);
}

std::shared_ptr<IntVarArray> FznTestBase::addIntVarArray(
    const std::vector<std::string>& identifiers,
    const std::string& identifier) {
  return _addIntVarArray(
      identifiers.empty()
          ? std::vector<IntArgState>{}
          : *rc::gen::container<std::vector<IntArgState>>(
                identifiers.size(), rc::gen::arbitrary<IntArgState>()),
      identifiers, identifier);
}

Arg FznTestBase::addArg(Int val) { return args.emplace_back(IntArg{val}); }
Arg FznTestBase::addArg(const std::vector<Int>& parameters,
                        const std::string& identifier) {
  auto i_par_arr = std::make_shared<IntVarArray>(identifier);
  for (const Int p : parameters) {
    i_par_arr->append(p);
  }
  args.emplace_back(i_par_arr);
  return i_par_arr;
}
Arg FznTestBase::addArg(const std::vector<bool>& parameters,
                        const std::string& identifier) {
  auto b_par_arr = std::make_shared<BoolVarArray>(identifier);
  for (bool parameter : parameters) {
    b_par_arr->append(parameter);
  }
  args.emplace_back(b_par_arr);
  return b_par_arr;
}

Arg FznTestBase::addIntSetArg(const Int lb, const Int ub,
                              const std::string& identifier) {
  std::vector<Int> elements(ub - lb + 1);
  std::iota(elements.begin(), elements.end(), lb);
  addIntSetPar(identifier, std::move(elements));
  return args.emplace_back(IntSetArg{IntSet{lb, ub}});
}

Arg FznTestBase::addIntSetArg(const std::vector<Int>& elements,
                              const std::string& identifier) {
  addIntSetPar(identifier, std::vector{elements});
  return args.emplace_back(IntSetArg{IntSet{std::vector{elements}}});
}

Arg FznTestBase::addIntSetArg(const std::string& identifier) {
  const size_t size = *rc::gen::inRange<size_t>(0, defaultUb + defaultLb + 2);
  auto elements = *rc::gen::unique<std::vector<Int>>(
      size, rc::gen::inRange<Int>(defaultLb, defaultUb + 1));
  addIntSetPar(identifier, std::vector{elements});
  return args.emplace_back(IntSetArg{IntSet{std::move(elements)}});
}

bool FznTestBase::randBool() { return binaryDist(gen) == 1; }

void FznTestBase::changeValue(const std::string& identifier,
                              const bool committedValue) {
  if (varNodeId(identifier) != NULL_NODE_ID) {
    changeValue(varNodeId(identifier), committedValue);
  }
}

void FznTestBase::changeValue(const VarNodeId vNodeId,
                              const bool committedValue) {
  if (varId(vNodeId) == propagation::NULL_ID) {
    return;
  }
  const auto& dom = _invariantGraph->varNodeConst(vNodeId).constDomain();
  if (dom->size() <= 1) {
    return;
  }
  const Int curVal = committedValue ? _solver->committedValue(varId(vNodeId))
                                    : _solver->currentValue(varId(vNodeId));
  const size_t offset =
      std::uniform_int_distribution<size_t>(0, dom->size() - 2)(gen);
  const Int newVal = *(dom->begin() + offset);
  RC_ASSERT(varId(vNodeId).isVar());
  _solver->setValue(varId(vNodeId),
                    curVal != newVal ? newVal : dom->upperBound());
}

void FznTestBase::markOutputVar(const std::string& identifier) {
  const auto vId = varNodeId(identifier);
  if (vId != NULL_NODE_ID) {
    _invariantGraph->varNode(vId).setIsOutputVar(true);
  }
}

void FznTestBase::rapidCheck(const bool reachesFixpoint,
                             const bool assumeCorrect) {
  bool neverSat = false;
  try {
    generate();
    neverSat = neverSatisfied();
    closeInvariantGraph();
    RC_ASSERT(_solverMapping != nullptr);
    RC_ASSERT(_solverMapping->globalNeighborhood() != nullptr);
    _assignment = std::make_shared<search::Assignment>(
        *_solver, _solverMapping->globalNeighborhood(),
        _solverMapping->totalViolationId(), _solverMapping->objectiveId(),
        ObjectiveDirection::NONE, 0);
    _randomProvider = std::make_shared<search::RandomProvider>(1234);
    _assignment->initialize(*_randomProvider);
  } catch (const InconsistencyException&) {
    if (assumeCorrect) {
      RC_SUCCEED();
    }
    RC_SUCCEED_IF(neverSat);
    RC_SUCCEED_IF(neverSatisfied());
    RC_FAIL();
  }
  if (neverSat) {
    if (assumeCorrect) {
      RC_SUCCEED();
    }
    RC_SUCCEED_IF(!reachesFixpoint);
    RC_SUCCEED_IF(!neverSatisfied());
    RC_FAIL();
  }
  RC_ASSERT(!neverSat);
  if (alwaysSatisfied()) {
    return;
  }

  if (!canMove()) {
    if (assumeCorrect) {
      RC_SUCCEED();
    }
    RC_ASSERT(isSatisfied(false));
    RC_ASSERT(isSatisfied(true));
    return;
  }
  constexpr size_t numCommits = 3;

  for (size_t c = 0; c < numCommits; ++c) {
    constexpr size_t numProbes = 3;
    RC_ASSERT(isSatisfied(true));

    for (size_t p = 0; p <= numProbes; ++p) {
      _solver->beginMove();
      move(p == numProbes);
      _solver->endMove();

      if (p == numProbes) {
        _solver->beginCommit();
      } else {
        _solver->beginProbe();
      }
      query();
      if (p == numProbes) {
        for (size_t i = 0;
             i < _invariantGraph->implicitConstraintNodes().size(); ++i) {
          RC_ASSERT(_solverMapping != nullptr);
          auto neighborhood =
              _solverMapping->neighborhood(InvariantNodeId(i, true));
          if (dynamic_cast<search::neighborhoods::NeighborhoodCombinator*>(
                  neighborhood.get()) != nullptr) {
            continue;
          }
          if (dynamic_cast<search::neighborhoods::RandomNeighborhood*>(
                  neighborhood.get()) != nullptr) {
            continue;
          }
          neighborhood->commitIf(*_assignment);
        }
        _solver->endCommit();
      } else {
        _solver->endProbe();
      }
      RC_ASSERT(isSatisfied(false));
    }
    RC_ASSERT(isSatisfied(true));
  }
}

std::ostream& operator<<(std::ostream& os, const BoolArgState state) {
  switch (state) {
    case BoolArgState::PAR_FALSE:
      return os << "BoolArgState::PAR_FALSE";
    case BoolArgState::PAR_TRUE:
      return os << "BoolArgState::PAR_TRUE";
    case BoolArgState::FIXED_FALSE:
      return os << "BoolArgState::FIXED_FALSE";
    case BoolArgState::FIXED_TRUE:
      return os << "BoolArgState::FIXED_TRUE";
    case BoolArgState::VAR:
    default:
      return os << "BoolArgState::VAR";
  }
}
std::ostream& operator<<(std::ostream& os, const IntArgState state) {
  switch (state) {
    case IntArgState::PAR:
      return os << "IntArgState::PAR";
    case IntArgState::FIXED:
      return os << "IntArgState::FIXED";
    case IntArgState::VAR:
    default:
      return os << "IntArgState::VAR";
  }
}
}  // namespace atlantis::testing
