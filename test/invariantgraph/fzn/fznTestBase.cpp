#include "./fznTestBase.hpp"

#include <rapidcheck/gen/Numeric.h>

#include <ranges>

#include "atlantis/search/neighborhoods/randomNeighborhood.hpp"
#include "atlantis/search/searchVariable.hpp"
#include "atlantis/utils/domains.hpp"

namespace atlantis::testing {
using namespace fznparser;
using namespace atlantis::invariantgraph;

std::string to_string(bool v) { return v ? "true" : "false"; }

void FznTestBase::SetUp() {
  _model = std::make_shared<Model>();
  _invariantGraph = std::make_shared<FznInvariantGraph>(true);
  _solver = nullptr;
  _solverMapping = nullptr;

  // Keep the non-RapidCheck move generation deterministic so failing property
  // cases can actually be reproduced from RC_PARAMS.
  gen = std::mt19937(0x5eed1234u);
  binaryDist = std::uniform_int_distribution<unsigned char>(0, 1);
  rapidcheck = true;
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
  if (rapidcheck) {
    RC_LOG() << "varNode(\"" << identifier << "\")" << std::endl;
    RC_ASSERT(_invariantGraph->containsVarNode(identifier));
  } else {
    EXPECT_TRUE(_invariantGraph->containsVarNode(identifier))
        << "varNode(\"" << identifier << "\")" << std::endl;
  }
  return _invariantGraph->varNode(identifier);
}
const VarNode& FznTestBase::varNodeConst(const std::string& identifier) const {
  if (rapidcheck) {
    RC_LOG() << "varNodeConst(\"" << identifier << "\")" << std::endl;
    RC_ASSERT(_invariantGraph->containsVarNode(identifier));
  } else {
    EXPECT_TRUE(_invariantGraph->containsVarNode(identifier))
        << "varNodeConst(\"" << identifier << "\")" << std::endl;
  }
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
             : propagation::NULL_ID;
}

void FznTestBase::setValue(const std::string& identifier, Int val) const {
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
  if (rapidcheck) {
    RC_LOG() << "unknown var \"" << identifier << "\"" << std::endl;
    RC_FAIL();
  } else {
    ADD_FAILURE() << "unknown var \"" << identifier << "\"" << std::endl;
  }
  return 0;
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
  if (rapidcheck) {
    RC_LOG() << "unknown var \"" << identifier << "\"" << std::endl;
    RC_FAIL();
  } else {
    ADD_FAILURE() << "unknown var \"" << identifier << "\"" << std::endl;
  }
  return 0;
}
bool FznTestBase::isFixed(const std::string& identifier) const {
  if (_invariantGraph->containsVarNode(identifier)) {
    return varNodeConst(identifier).isFixed();
  }
  if (boolPars.contains(identifier) || intPars.contains(identifier)) {
    return true;
  }
  if (rapidcheck) {
    RC_LOG() << "unknown var \"" << identifier << "\"" << std::endl;
    RC_FAIL();
  } else {
    ADD_FAILURE() << "unknown var \"" << identifier << "\"" << std::endl;
    return false;
  }
}
bool FznTestBase::boolVal(const std::string& identifier,
                          bool committedValue) const {
  std::optional<bool> ret{};
  if (_invariantGraph->containsVarNode(identifier)) {
    const auto& vNode = varNodeConst(identifier);
    if (rapidcheck) {
      RC_ASSERT_FALSE(vNode.isIntVar());
    } else {
      EXPECT_FALSE(vNode.isIntVar());
    }
    if (_solverMapping != nullptr &&
        _solverMapping->solverId(vNode.varNodeId()) != propagation::NULL_ID) {
      ret = (committedValue ? _solver->committedValue(
                                  _solverMapping->solverId(vNode.varNodeId()))
                            : _solver->currentValue(_solverMapping->solverId(
                                  vNode.varNodeId()))) == 0;
    } else if (vNode.constDomain()->size() > 0) {
      if (rapidcheck) {
        RC_ASSERT(vNode.isFixed());
      } else {
        EXPECT_TRUE(vNode.isFixed());
      }
      ret = vNode.inDomain(bool{true});
    }
  } else if (boolPars.contains(identifier)) {
    ret = boolPars.at(identifier);
  }
  if (ret.has_value()) {
    if (rapidcheck) {
      RC_LOG() << "boolVal(\"" << identifier
               << "\", committedValue=" << to_string(committedValue)
               << ") = " << to_string(*ret) << std::endl;
    }
    return *ret;
  }
  if (rapidcheck) {
    RC_LOG() << "boolVal(\"" << identifier
             << "\", committedValue=" << to_string(committedValue)
             << ") unhandled argument type" << std::endl;
    RC_FAIL();
  } else {
    ADD_FAILURE() << "boolVal(\"" << identifier
                  << "\", committedValue=" << to_string(committedValue)
                  << ") unhandled argument type" << std::endl;
    return false;
  }
}
bool FznTestBase::inDomain(const std::string& identifier, bool val) const {
  if (rapidcheck) {
    RC_LOG() << "inDomain(\"" << identifier << "\", " << to_string(val) << ')'
             << std::endl;
  }
  if (_invariantGraph->containsVarNode(identifier)) {
    const auto& vNode = varNodeConst(identifier);
    if (rapidcheck) {
      RC_ASSERT_FALSE(vNode.isIntVar());
    } else {
      EXPECT_FALSE(vNode.isIntVar());
    }
    return vNode.inDomain(val);
  }
  if (boolPars.contains(identifier)) {
    return boolPars.at(identifier) == val;
  }
  if (rapidcheck) {
    RC_LOG() << "unhandled argument type" << std::endl;
    RC_FAIL();
  } else {
    ADD_FAILURE() << "inDomain(\"" << identifier << "\", " << to_string(val)
                  << ')' << std::endl
                  << "unhandled argument type" << std::endl;
    return false;
  }
}
bool FznTestBase::isFixedTo(const std::string& identifier, bool val) const {
  if (isFixed(identifier)) {
    return boolVal(identifier, val) == val;
  }
  return false;
}
bool FznTestBase::isFixedTo(const std::string& identifier, Int val) const {
  if (isFixed(identifier)) {
    return intVal(identifier, val) == val;
  }
  return false;
}
Int FznTestBase::intVal(const std::string& identifier,
                        bool committedValue) const {
  std::optional<Int> ret{};
  if (_invariantGraph->containsVarNode(identifier)) {
    const auto& vNode = varNodeConst(identifier);
    if (rapidcheck) {
      RC_ASSERT(vNode.isIntVar());
    } else {
      EXPECT_TRUE(vNode.isIntVar());
    }
    if (_solverMapping != nullptr &&
        _solverMapping->solverId(vNode.varNodeId()) != propagation::NULL_ID) {
      ret = committedValue ? _solver->committedValue(
                                 _solverMapping->solverId(vNode.varNodeId()))
                           : _solver->currentValue(
                                 _solverMapping->solverId(vNode.varNodeId()));
    } else {
      if (rapidcheck) {
        RC_ASSERT(vNode.isFixed());
      } else {
        EXPECT_TRUE(vNode.isFixed());
      }
      ret = vNode.lowerBound();
    }
  } else if (intPars.contains(identifier)) {
    ret = intPars.at(identifier);
  }
  if (ret.has_value()) {
    if (rapidcheck) {
      RC_LOG() << "intVal(\"" << identifier
               << "\", committedValue=" << to_string(committedValue)
               << ") = " << *ret << std::endl;
    }
    return *ret;
  }
  if (rapidcheck) {
    RC_LOG() << "intVal(\"" << identifier
             << "\", committedValue=" << to_string(committedValue)
             << ") unhandled argument type" << std::endl;
    RC_FAIL();
  } else {
    ADD_FAILURE() << "intVal(\"" << identifier
                  << "\", committedValue=" << to_string(committedValue)
                  << ") unhandled argument type" << std::endl;
    return 0;
  }
}

bool FznTestBase::inDomain(const std::string& identifier, Int val) const {
  if (rapidcheck) {
    RC_LOG() << "inDomain(\"" << identifier << "\", " << val << ')'
             << std::endl;
  }
  if (_invariantGraph->containsVarNode(identifier)) {
    const auto& vNode = varNodeConst(identifier);
    if (rapidcheck) {
      RC_ASSERT(vNode.isIntVar());
    } else {
      EXPECT_TRUE(vNode.isIntVar());
    }
    return vNode.inDomain(val);
  }
  if (intPars.contains(identifier)) {
    return intPars.at(identifier) == val;
  }
  if (rapidcheck) {
    RC_LOG() << "unhandled argument type" << std::endl;
    RC_FAIL();
  } else {
    ADD_FAILURE() << "inDomain(\"" << identifier << "\", " << val << ')'
                  << std::endl
                  << "unhandled argument type" << std::endl;
    return false;
  }
}

const std::vector<Int>& FznTestBase::intSetVal(
    const std::string& identifier) const {
  if (rapidcheck) {
    RC_LOG() << "intSetVal(\"" << identifier << ')' << std::endl;
  }
  if (intSetPars.contains(identifier)) {
    return intSetPars.at(identifier);
  }
  if (rapidcheck) {
    RC_LOG() << "unhandled argument type" << std::endl;
    RC_FAIL();
  } else {
    ADD_FAILURE() << "intSetVal(\"" << identifier << ')' << std::endl
                  << "unhandled argument type" << std::endl;
    throw std::runtime_error("");
  }
}

propagation::VarViewId FznTestBase::totalViolationVarId() const {
  return _solverMapping == nullptr ? propagation::NULL_ID
                                   : _solverMapping->totalViolationId();
}
Int FznTestBase::violation(bool committedValue) const {
  if (totalViolationVarId() == propagation::NULL_ID) {
    return 0;
  }
  return committedValue ? _solver->committedValue(totalViolationVarId())
                        : _solver->currentValue(totalViolationVarId());
}
std::shared_ptr<IntVar> FznTestBase::genIntVar(Int lb, Int ub,
                                               const std::string& identifier) {
  if (lb == ub) {
    addIntPar(identifier, lb);
  }
  return std::get<std::shared_ptr<IntVar>>(
      _model->addVar(std::make_shared<IntVar>(lb, ub, identifier)));
}

std::shared_ptr<IntVar> FznTestBase::genIntVar(const std::vector<Int>& dom,
                                               const std::string& identifier) {
  if (rapidcheck) {
    RC_ASSERT_FALSE(dom.empty());
  } else {
    EXPECT_FALSE(dom.empty());
  }
  if (dom.size() == 1) {
    addIntPar(identifier, dom.front());
  }
  return std::get<std::shared_ptr<IntVar>>(
      _model->addVar(std::make_shared<IntVar>(std::vector{dom}, identifier)));
}

std::shared_ptr<IntVar> FznTestBase::genIntVar(const std::string& identifier) {
  return genIntVar(defaultLb, defaultUb, identifier);
}
std::shared_ptr<IntVar> FznTestBase::genIntVar(IntArgState state,
                                               const std::string& identifier) {
  return genIntVar(state, defaultLb, defaultUb, identifier);
}
std::shared_ptr<IntVar> FznTestBase::genIntVar(IntArgState state, Int lb,
                                               Int ub,
                                               const std::string& identifier) {
  switch (state) {
    case IntArgState::FIXED: {
      const Int val = lb == ub ? lb : *rc::gen::inRange<Int>(lb, ub + 1);
      return genIntVar(val, val, identifier);
    }
    case IntArgState::VAR: {
      return genIntVar(lb, ub, identifier);
    }
    default: {
      if (rapidcheck) {
        RC_LOG() << "unhandled IntArgState" << std::endl;
        RC_FAIL();
      } else {
        ADD_FAILURE() << "unhandled IntArgState" << std::endl;
      }
    }
  }
}
void FznTestBase::addBoolPar(const std::string& identifier, bool val) {
  if (rapidcheck) {
    RC_LOG() << "addBoolPar(\"" << identifier << "\", " << to_string(val) << ')'
             << std::endl;
    RC_ASSERT_FALSE(identifier.empty());
    RC_ASSERT_FALSE(boolPars.contains(identifier));
    RC_ASSERT_FALSE(intPars.contains(identifier));
    RC_ASSERT_FALSE(intSetPars.contains(identifier));
  } else {
    EXPECT_FALSE(identifier.empty()) << "addBoolPar(\"" << identifier << "\", "
                                     << to_string(val) << ')' << std::endl;
    EXPECT_FALSE(boolPars.contains(identifier))
        << "addBoolPar(\"" << identifier << "\", " << to_string(val) << ')'
        << std::endl;
    EXPECT_FALSE(intPars.contains(identifier))
        << "addBoolPar(\"" << identifier << "\", " << to_string(val) << ')'
        << std::endl;
    EXPECT_FALSE(intSetPars.contains(identifier))
        << "addBoolPar(\"" << identifier << "\", " << to_string(val) << ')'
        << std::endl;
  }
  boolPars.emplace(identifier, val);
}

void FznTestBase::addIntPar(const std::string& identifier, Int val) {
  if (rapidcheck) {
    RC_LOG() << "addIntPar(\"" << identifier << "\", " << val << ')'
             << std::endl;
    RC_ASSERT_FALSE(identifier.empty());
    RC_ASSERT_FALSE(boolPars.contains(identifier));
    RC_ASSERT_FALSE(intPars.contains(identifier));
    RC_ASSERT_FALSE(intSetPars.contains(identifier));
  } else {
    EXPECT_FALSE(identifier.empty())
        << "addIntPar(\"" << identifier << "\", " << val << ')' << std::endl;
    EXPECT_FALSE(boolPars.contains(identifier))
        << "addIntPar(\"" << identifier << "\", " << val << ')' << std::endl;
    EXPECT_FALSE(intPars.contains(identifier))
        << "addIntPar(\"" << identifier << "\", " << val << ')' << std::endl;
    EXPECT_FALSE(intSetPars.contains(identifier))
        << "addIntPar(\"" << identifier << "\", " << val << ')' << std::endl;
  }
  intPars.emplace(identifier, val);
}

void FznTestBase::addIntSetPar(const std::string& identifier,
                               std::vector<Int>&& vals) {
  std::string msg = "addIntSetPar(\"" + identifier + "\", {";
  for (size_t i = 0; i < vals.size(); ++i) {
    msg += (i != 0 ? "" : ", ") + vals.at(i);
  }
  msg += "})";
  if (rapidcheck) {
    RC_LOG() << msg << std::endl;
    RC_ASSERT_FALSE(identifier.empty());
    RC_ASSERT_FALSE(boolPars.contains(identifier));
    RC_ASSERT_FALSE(intPars.contains(identifier));
    RC_ASSERT_FALSE(intSetPars.contains(identifier));
  } else {
    EXPECT_FALSE(identifier.empty()) << msg << std::endl;
    EXPECT_FALSE(boolPars.contains(identifier)) << msg << std::endl;
    EXPECT_FALSE(intPars.contains(identifier)) << msg << std::endl;
    EXPECT_FALSE(intSetPars.contains(identifier)) << msg << std::endl;
  }
  intSetPars.emplace(identifier, std::move(vals));
}

IntArg FznTestBase::addIntArg(IntArgState state, Int lb, Int ub,
                              const std::string& identifier) {
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
    default: {
      if (rapidcheck) {
        RC_LOG() << "Invalid IntArgState" << std::endl;
        RC_FAIL();
      } else {
        ADD_FAILURE() << "Invalid IntArgState" << std::endl;
        return {};
      }
    }
  }
}

IntArg FznTestBase::addIntArg(IntArgState state, const std::vector<Int>& dom,
                              const std::string& identifier) {
  if (rapidcheck) {
    RC_ASSERT_FALSE(dom.empty());
  } else {
    EXPECT_FALSE(dom.empty());
  }
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
      if (rapidcheck) {
        RC_ASSERT(dom.size() > size_t{1});
      } else {
        EXPECT_GT(dom.size(), size_t{1});
      }
      auto var = genIntVar(dom, identifier);
      args.emplace_back(var);
      return var;
    }
    default: {
      if (rapidcheck) {
        RC_LOG() << "Invalid IntArgState" << std::endl;
        RC_FAIL();
      } else {
        ADD_FAILURE() << "Invalid IntArgState" << std::endl;
        return {};
      }
    }
  }
}

IntArg FznTestBase::addIntArg(IntArgState state,
                              const std::string& identifier) {
  return addIntArg(state, defaultLb, defaultUb, identifier);
}
IntArg FznTestBase::addIntArg(Int lb, Int ub, const std::string& identifier) {
  return addIntArg(lb == ub
                       ? *rc::gen::element(IntArgState::PAR, IntArgState::FIXED)
                       : *rc::gen::arbitrary<IntArgState>(),
                   lb, ub, identifier);
}
IntArg FznTestBase::addIntArg(const std::string& identifier) {
  return addIntArg(defaultLb, defaultUb, identifier);
}
std::shared_ptr<IntVarArray> FznTestBase::genIntParArray(
    size_t arraySize, Int lb, Int ub, const std::string& identifier) {
  auto parArray = std::get<std::shared_ptr<IntVarArray>>(
      _model->addVar(std::make_shared<IntVarArray>(identifier)));
  const std::vector<Int> pars = *rc::gen::container<std::vector<Int>>(
      arraySize, rc::gen::inRange<Int>(lb, ub));
  for (size_t i = 0; i < arraySize; ++i) {
    parArray->append(pars.at(i));
  }
  return parArray;
}
std::shared_ptr<IntVarArray> FznTestBase::genIntVarArray(
    size_t arraySize, Int lb, Int ub, const std::string& identifier,
    const std::string& varPrefix) {
  auto vars = std::get<std::shared_ptr<IntVarArray>>(
      _model->addVar(std::make_shared<IntVarArray>(identifier)));
  std::vector<IntArgState> argStates =
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
      default: {
        if (rapidcheck) {
          RC_LOG() << "Invalid IntArgState" << std::endl;
          RC_FAIL();
        } else {
          ADD_FAILURE() << "Invalid IntArgState" << std::endl;
          return {};
        }
      }
    }
  }
  return vars;
}
std::shared_ptr<IntVarArray> FznTestBase::genIntVarArray(
    size_t arraySize, const std::string& identifier,
    const std::string& varPrefix) {
  return genIntVarArray(arraySize, defaultLb, defaultUb, identifier, varPrefix);
}
std::shared_ptr<IntVarArray> FznTestBase::genIntParArray(
    size_t arraySize, const std::string& identifier) {
  return genIntParArray(arraySize, defaultLb, defaultUb, identifier);
}
std::shared_ptr<BoolVar> FznTestBase::genBoolVar(
    BoolArgState state, const std::string& identifier) {
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
    default: {
      if (rapidcheck) {
        RC_LOG() << "Invalid BoolArgState" << std::endl;
        RC_FAIL();
      } else {
        ADD_FAILURE() << "Invalid BoolArgState" << std::endl;
        return {};
      }
    }
  }
}

std::vector<Int> FznTestBase::genDomain(size_t size) const {
  if (rapidcheck) {
    RC_ASSERT(size < static_cast<size_t>(defaultUb - defaultLb + 2));
  } else {
    EXPECT_LT(size, static_cast<size_t>(defaultUb - defaultLb + 2));
  }
  return *rc::gen::unique<std::vector<Int>>(
      size, rc::gen::inRange<Int>(defaultLb, defaultUb + 1));
}

std::vector<Int> FznTestBase::genDomain(IntArgState state) const {
  const size_t size =
      state != IntArgState::VAR
          ? 1
          : *rc::gen::inRange<size_t>(2, defaultUb - defaultLb + 2);
  return genDomain(size);
}

std::vector<Int> FznTestBase::genDomain() const {
  return genDomain(*rc::gen::arbitrary<IntArgState>());
}

BoolArg FznTestBase::addBoolArg(BoolArgState state,
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
BoolArg FznTestBase::addBoolArg(const std::string& identifier) {
  return addBoolArg(*rc::gen::arbitrary<BoolArgState>(), identifier);
}
std::shared_ptr<BoolVarArray> FznTestBase::genBoolParArray(
    size_t arraySize, const std::string& identifier) {
  std::vector<bool> pars = *rc::gen::container<std::vector<bool>>(
      arraySize, rc::gen::arbitrary<bool>());
  auto argArray = std::get<std::shared_ptr<BoolVarArray>>(
      _model->addVar(std::make_shared<BoolVarArray>(identifier)));
  for (size_t i = 0; i < arraySize; ++i) {
    argArray->append(pars.at(i));
  }
  return argArray;
}
std::shared_ptr<BoolVarArray> FznTestBase::addBoolVarArray(
    size_t arraySize, const std::string& identifier,
    const std::string& varPrefix) {
  std::vector<BoolArgState> argStates =
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
    const std::vector<std::string>& identifiers,
    const std::string& identifier) {
  std::vector<BoolArgState> argStates =
      *rc::gen::container<std::vector<BoolArgState>>(
          identifiers.size(), rc::gen::arbitrary<BoolArgState>());
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
  if (rapidcheck) {
    RC_ASSERT(argStates.size() == identifiers.size());
  } else {
    EXPECT_EQ(argStates.size(), identifiers.size());
  }
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

std::shared_ptr<IntVarArray> FznTestBase::addIntVarArray(
    const std::vector<IntArgState>& argStates,
    const std::vector<std::pair<Int, Int>>& domains,
    const std::vector<std::string>& identifiers,
    const std::string& identifier) {
  if (rapidcheck) {
    RC_ASSERT(argStates.size() == identifiers.size());
    RC_ASSERT(domains.size() == identifiers.size());
  } else {
    EXPECT_EQ(argStates.size(), identifiers.size());
    EXPECT_EQ(domains.size(), identifiers.size());
  }
  auto vars = std::get<std::shared_ptr<IntVarArray>>(
      _model->addVar(std::make_shared<IntVarArray>(identifier)));
  for (size_t i = 0; i < identifiers.size(); ++i) {
    const auto [lb, ub] = domains.at(i);
    if (rapidcheck) {
      RC_ASSERT(lb <= ub);
    } else {
      EXPECT_LE(lb, ub);
    }
    switch (argStates.at(i)) {
      case IntArgState::PAR: {
        const Int val = lb == ub ? lb : *rc::gen::inRange<Int>(lb, ub + 1);
        vars->append(val);
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
    const std::vector<std::string>& identifiers,
    const std::string& identifier) {
  if (rapidcheck) {
    RC_ASSERT(argStates.size() == identifiers.size());
  } else {
    EXPECT_EQ(argStates.size(), identifiers.size());
  }
  auto vars = std::get<std::shared_ptr<IntVarArray>>(
      _model->addVar(std::make_shared<IntVarArray>(identifier)));
  for (size_t i = 0; i < identifiers.size(); ++i) {
    switch (argStates.at(i)) {
      case IntArgState::PAR: {
        const Int val = *rc::gen::inRange<Int>(defaultLb, defaultUb + 1);
        vars->append(val);
        addIntPar(identifiers.at(i), val);
        break;
      }
      default:
        vars->append(genIntVar(argStates.at(i), identifiers.at(i)));
        break;
    }
  }
  args.emplace_back(vars);
  return vars;
}
std::shared_ptr<IntVarArray> FznTestBase::addIntVarArray(
    const std::vector<std::string>& identifiers,
    const std::string& identifier) {
  return addIntVarArray(
      identifiers.empty()
          ? std::vector<IntArgState>{}
          : *rc::gen::container<std::vector<IntArgState>>(
                identifiers.size(), rc::gen::arbitrary<IntArgState>()),
      identifiers, identifier);
}
std::shared_ptr<IntVarArray> FznTestBase::addIntVarArray(
    size_t arraySize, const std::string& identifier,
    const std::string& varPrefix) {
  std::vector<std::string> identifiers(arraySize);
  for (size_t i = 0; i < arraySize; ++i) {
    identifiers.at(i) = varPrefix + std::to_string(i);
  }
  return addIntVarArray(arraySize == 0
                            ? std::vector<IntArgState>{}
                            : *rc::gen::container<std::vector<IntArgState>>(
                                  arraySize, rc::gen::arbitrary<IntArgState>()),
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
  for (size_t i = 0; i < parameters.size(); ++i) {
    b_par_arr->append(parameters.at(i));
  }
  args.emplace_back(b_par_arr);
  return b_par_arr;
}

Arg FznTestBase::addIntSetArg(Int lb, Int ub, const std::string& identifier) {
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
                              bool committedValue) {
  if (varId(identifier) == propagation::NULL_ID) {
    return;
  }
  const auto& dom = varNodeConst(identifier).constDomain();
  if (dom->size() <= 1) {
    return;
  }
  const Int curVal = committedValue ? _solver->committedValue(varId(identifier))
                                    : _solver->currentValue(varId(identifier));
  const size_t offset =
      std::uniform_int_distribution<size_t>(0, dom->size() - 2)(gen);
  const Int newVal = *(dom->begin() + offset);
  _solver->setValue(varId(identifier),
                    curVal != newVal ? newVal : dom->upperBound());
}

void FznTestBase::rapidCheck(bool reachesFixpoint) {
  RC_ASSERT(rapidcheck);
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
    RC_SUCCEED_IF(neverSat);
    RC_SUCCEED_IF(neverSatisfied());
    RC_FAIL();
  }
  if (neverSat) {
    RC_SUCCEED_IF(!reachesFixpoint);
    RC_SUCCEED_IF(!neverSatisfied());
    RC_FAIL();
  }
  RC_ASSERT_FALSE(neverSat);
  if (alwaysSatisfied()) {
    return;
  }

  if (!canMove()) {
    RC_ASSERT(isSatisfied(false));
    RC_ASSERT(isSatisfied(true));
    return;
  }
  constexpr size_t numCommits = 3;
  constexpr size_t numProbes = 3;

  for (size_t c = 0; c < numCommits; ++c) {
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

}  // namespace atlantis::testing
