#pragma once

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <rapidcheck/gen/Numeric.h>
#include <rapidcheck/gtest.h>

#include <boost/spirit/home/classic/phoenix/primitives.hpp>
#include <fznparser/constraint.hpp>
#include <fznparser/model.hpp>
#include <fznparser/variables.hpp>
#include <random>
#include <string>
#include <vector>

#include "atlantis/invariantgraph/fznInvariantGraph.hpp"
#include "atlantis/invariantgraph/varNode.hpp"
#include "atlantis/propagation/solver.hpp"

namespace atlantis::testing {
using namespace fznparser;
using namespace atlantis::invariantgraph;

enum struct BoolArgState : unsigned char {
  PAR_FALSE = 0,
  PAR_TRUE = 1,
  FIXED_FALSE = 2,
  FIXED_TRUE = 3,
  VAR = 4
};
enum struct IntArgState : unsigned char { PAR = 0, FIXED = 1, VAR = 2 };

inline std::ostream &operator<<(std::ostream &os, BoolArgState state) {
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

inline std::ostream &operator<<(std::ostream &os, IntArgState state) {
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

}

namespace rc {
  using namespace atlantis::testing;
  template <>
  struct Arbitrary<BoolArgState> {
    static Gen<BoolArgState> arbitrary() {
      return gen::element<BoolArgState>(BoolArgState::PAR_FALSE, BoolArgState::PAR_TRUE, BoolArgState::FIXED_FALSE, BoolArgState::FIXED_TRUE, BoolArgState::VAR);
    }
  };
template <>
  struct Arbitrary<IntArgState> {
  static Gen<IntArgState> arbitrary() {
    return gen::element<IntArgState>(IntArgState::PAR, IntArgState::FIXED, IntArgState::VAR);
  }
};
}

namespace atlantis::testing {

struct ArgState {
  std::variant<BoolArgState, IntArgState> state;
  bool isArray{false};
  bool isOptional{false};

  explicit ArgState(std::variant<BoolArgState, IntArgState> _state,
                    bool _isArray = false, bool _isOptional = false)
      : state(_state), isArray(_isArray), isOptional(_isOptional) {}
};

class FznTestBase : public ::testing::Test {
 public:
  std::shared_ptr<Model> _model;
  std::shared_ptr<FznInvariantGraph> _invariantGraph;
  std::shared_ptr<propagation::Solver> _solver;
  std::string constraintIdentifier;
  std::vector<ArgState> argStates;
  std::vector<Arg> args;
  std::mt19937 gen;
  std::default_random_engine rng;
  std::uniform_int_distribution<unsigned char> binaryDist;
  std::unordered_map<std::string, Int> intPars;
  std::unordered_map<std::string, bool> boolPars;

  const Int defaultLb = -1;
  const Int defaultUb = 1;

  void SetUp() override {
    _model = std::make_shared<Model>();
    _solver = std::make_shared<propagation::Solver>();
    _invariantGraph = std::make_shared<FznInvariantGraph>(*_solver, true);

    std::random_device rd;
    gen = std::mt19937(rd());
    binaryDist = std::uniform_int_distribution<unsigned char>(0, 1);
  }

  void generateConstraint() {
    _model->addConstraint(
        Constraint{constraintIdentifier, std::vector<Arg>{args}});
    _invariantGraph->build(*_model);
  }

  virtual void generate() = 0;
  [[nodiscard]] virtual bool isSatisfied(bool committedValue) const = 0;
  [[nodiscard]] virtual bool alwaysSatisfied() const { return false; };
  [[nodiscard]] virtual bool neverSatisfied() const { return false; };
  virtual bool canMove() const = 0;
  virtual void move(bool committedValue) = 0;
  virtual void query() = 0;

  [[nodiscard]] const std::string& identifier(size_t argIndex) const {
    if (std::holds_alternative<BoolArg>(args.at(argIndex))) {
      const auto& arg = std::get<BoolArg>(args.at(argIndex));
      if (!arg.isParameter()) {
        return std::get<BoolArg>(args.at(argIndex)).var()->identifier();
      }
    } else if (std::holds_alternative<IntArg>(args.at(argIndex))) {
      const auto& arg = std::get<IntArg>(args.at(argIndex));
      if (!arg.isParameter()) {
        return std::get<BoolArg>(args.at(argIndex)).var()->identifier();
      }
    }
    if (std::holds_alternative<FloatArg>(args.at(argIndex))) {
      const auto& arg = std::get<FloatArg>(args.at(argIndex));
      if (!arg.isParameter()) {
        return std::get<BoolArg>(args.at(argIndex)).var()->identifier();
      }
    }
    if (std::holds_alternative<IntSetArg>(args.at(argIndex))) {
      const auto& arg = std::get<IntSetArg>(args.at(argIndex));
      if (!arg.isParameter()) {
        return std::get<BoolArg>(args.at(argIndex)).var()->identifier();
      }
    }
    if (std::holds_alternative<std::shared_ptr<BoolVarArray>>(
            args.at(argIndex))) {
      return std::get<std::shared_ptr<BoolVarArray>>(args.at(argIndex))
          ->identifier();
    }
    if (std::holds_alternative<std::shared_ptr<IntVarArray>>(
            args.at(argIndex))) {
      return std::get<std::shared_ptr<IntVarArray>>(args.at(argIndex))
          ->identifier();
    }
    if (std::holds_alternative<std::shared_ptr<FloatVarArray>>(
            args.at(argIndex))) {
      return std::get<std::shared_ptr<FloatVarArray>>(args.at(argIndex))
          ->identifier();
    }
    if (std::holds_alternative<std::shared_ptr<SetVarArray>>(
            args.at(argIndex))) {
      return std::get<std::shared_ptr<SetVarArray>>(args.at(argIndex))
          ->identifier();
    }
    throw std::logic_error("Unhandled argument type");
  }

  [[nodiscard]] bool boolPar(size_t argIndex) const {
    if (std::holds_alternative<BoolArg>(args.at(argIndex)) &&
        std::get<BoolArg>(args.at(argIndex)).isParameter()) {
      return std::get<BoolArg>(args.at(argIndex)).parameter();
    }
    throw std::logic_error("Unhandled argument type");
  }

  [[nodiscard]] bool boolPar(size_t argIndex, size_t vectorIndex) const {
    if (std::holds_alternative<std::shared_ptr<BoolVarArray>>(
            args.at(argIndex))) {
      const auto& varArray =
          std::get<std::shared_ptr<BoolVarArray>>(args.at(argIndex));
      if (std::holds_alternative<bool>(varArray->at(vectorIndex))) {
        return std::get<bool>(varArray->at(vectorIndex));
      }
    }
    throw std::logic_error("Unhandled argument type");
  }

  [[nodiscard]] bool intPar(size_t argIndex) const {
    if (std::holds_alternative<BoolArg>(args.at(argIndex)) &&
        std::get<BoolArg>(args.at(argIndex)).isParameter()) {
      return std::get<BoolArg>(args.at(argIndex)).parameter();
    }
    throw std::logic_error("Unhandled argument type");
  }

  [[nodiscard]] Int intPar(size_t argIndex, size_t vectorIndex) const {
    if (std::holds_alternative<std::shared_ptr<IntVarArray>>(
            args.at(argIndex))) {
      const auto& varArray =
          std::get<std::shared_ptr<IntVarArray>>(args.at(argIndex));
      if (std::holds_alternative<Int>(varArray->at(vectorIndex))) {
        return std::get<Int>(varArray->at(vectorIndex));
      }
    }
    throw std::logic_error("Unhandled argument type");
  }

  [[nodiscard]] double floatPar(size_t argIndex) const {
    if (std::holds_alternative<FloatArg>(args.at(argIndex)) &&
        std::get<FloatArg>(args.at(argIndex)).isParameter()) {
      return std::get<FloatArg>(args.at(argIndex)).parameter();
    }
    throw std::logic_error("Unhandled argument type");
  }

  [[nodiscard]] double floatPar(size_t argIndex, size_t vectorIndex) const {
    if (std::holds_alternative<std::shared_ptr<FloatVarArray>>(
            args.at(argIndex))) {
      const auto& varArray =
          std::get<std::shared_ptr<FloatVarArray>>(args.at(argIndex));
      if (std::holds_alternative<double>(varArray->at(vectorIndex))) {
        return std::get<double>(varArray->at(vectorIndex));
      }
    }
    throw std::logic_error("Unhandled argument type");
  }

  [[nodiscard]] const IntSet& intSetPar(size_t argIndex) const {
    if (std::holds_alternative<IntSetArg>(args.at(argIndex)) &&
        std::get<IntSetArg>(args.at(argIndex)).isParameter()) {
      return std::get<IntSetArg>(args.at(argIndex)).parameter();
    }
    throw std::logic_error("Unhandled argument type");
  }

  [[nodiscard]] const IntSet& intSetPar(size_t argIndex,
                                        size_t vectorIndex) const {
    if (std::holds_alternative<std::shared_ptr<SetVarArray>>(
            args.at(argIndex))) {
      const auto& varArray =
          std::get<std::shared_ptr<SetVarArray>>(args.at(argIndex));
      if (std::holds_alternative<IntSet>(varArray->at(argIndex))) {
        return std::get<IntSet>(varArray->at(vectorIndex));
      }
    }
    throw std::logic_error("Unhandled argument type");
  }

  [[nodiscard]] const std::string& identifier(size_t argIndex,
                                              size_t vectorIndex) const {
    if (std::holds_alternative<std::shared_ptr<BoolVarArray>>(
            args.at(argIndex))) {
      const auto& varArray =
          std::get<std::shared_ptr<BoolVarArray>>(args.at(argIndex));
      if (std::holds_alternative<std::shared_ptr<const BoolVar>>(
              varArray->at(argIndex))) {
        return std::get<std::shared_ptr<const BoolVar>>(
                   varArray->at(vectorIndex))
            ->identifier();
      }
    } else if (std::holds_alternative<std::shared_ptr<IntVarArray>>(
                   args.at(argIndex))) {
      const auto& varArray =
          std::get<std::shared_ptr<IntVarArray>>(args.at(argIndex));
      if (std::holds_alternative<std::shared_ptr<const IntVar>>(
              varArray->at(argIndex))) {
        return std::get<std::shared_ptr<const IntVar>>(
                   varArray->at(vectorIndex))
            ->identifier();
      }
    } else if (std::holds_alternative<std::shared_ptr<FloatVarArray>>(
                   args.at(argIndex))) {
      const auto& varArray =
          std::get<std::shared_ptr<FloatVarArray>>(args.at(argIndex));
      if (std::holds_alternative<std::shared_ptr<const FloatVar>>(
              varArray->at(argIndex))) {
        return std::get<std::shared_ptr<const FloatVar>>(
                   varArray->at(vectorIndex))
            ->identifier();
      }
    } else if (std::holds_alternative<std::shared_ptr<SetVarArray>>(
                   args.at(argIndex))) {
      const auto& varArray =
          std::get<std::shared_ptr<SetVarArray>>(args.at(argIndex));
      if (std::holds_alternative<std::shared_ptr<const SetVar>>(
              varArray->at(argIndex))) {
        return std::get<std::shared_ptr<const SetVar>>(
                   varArray->at(vectorIndex))
            ->identifier();
      }
    }
    throw std::logic_error("Unhandled argument type");
  }

  [[nodiscard]] VarNodeId varNodeId(const std::string& identifier) const {
    if (_invariantGraph->containsVarNode(identifier)) {
      return _invariantGraph->varNodeId(identifier);
    }
    return NULL_NODE_ID;
  }

  [[nodiscard]] propagation::VarViewId varId(
      const std::string& identifier) const {
    if (_invariantGraph->containsVarNode(identifier)) {
      return _invariantGraph->varId(identifier);
    }
    return propagation::NULL_ID;
  }

  void setValue(const std::string& identifier, Int val) const {
    _solver->setValue(varId(identifier), val);
  }

  [[nodiscard]] Int currentValue(const std::string& identifier) const {
    return _solver->currentValue(varId(identifier));
  }

  [[nodiscard]] Int lowerBound(const std::string& identifier) const {
    if (_invariantGraph->containsVarNode(identifier)) {
      return _invariantGraph->varNodeConst(identifier).lowerBound();
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
    throw std::logic_error("Unhandled argument type");
  }

  [[nodiscard]] Int upperBound(const std::string& identifier) const {
    if (_invariantGraph->containsVarNode(identifier)) {
      return _invariantGraph->varNodeConst(identifier).upperBound();
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
    throw std::logic_error("Unhandled argument type");
  }

  [[nodiscard]] bool isFixed(const std::string& identifier) const {
    if (_invariantGraph->containsVarNode(identifier)) {
      return _invariantGraph->varNodeConst(identifier).isFixed();
    }
    if (boolPars.contains(identifier) || intPars.contains(identifier)) {
      return true;
    }
    throw std::logic_error("Unhandled argument type");
  }

  [[nodiscard]] bool boolVal(const std::string& identifier,
                             bool committedValue = false) const {
    if (_invariantGraph->containsVarNode(identifier)) {
      const auto& vNode = _invariantGraph->varNodeConst(identifier);
      assert(!vNode.isIntVar());
      if (vNode.varId() != propagation::NULL_ID) {
        return (committedValue ? _solver->committedValue(vNode.varId())
                               : _solver->currentValue(vNode.varId())) == 0;
      }
      assert(vNode.isFixed());
      return vNode.inDomain(bool{true});
    }
    if (boolPars.contains(identifier)) {
      return boolPars.at(identifier);
    }
    throw std::logic_error("Unhandled argument type");
  }

  [[nodiscard]] bool inDomain(const std::string& identifier, bool val) const {
    if (_invariantGraph->containsVarNode(identifier)) {
      const auto& vNode = _invariantGraph->varNodeConst(identifier);
      assert(!vNode.isIntVar());
      return vNode.inDomain(val);
    }
    if (boolPars.contains(identifier)) {
      return boolPars.at(identifier) == val;
    }
    throw std::logic_error("Unhandled argument type");
  }

  [[nodiscard]] bool isFixedTo(const std::string& identifier, bool val) const {
    if (isFixed(identifier)) {
      return boolVal(identifier, val) == val;
    }
    return false;
  }

  [[nodiscard]] bool isFixedTo(const std::string& identifier, Int val) const {
    if (isFixed(identifier)) {
      return intVal(identifier, val) == val;
    }
    return false;
  }

  [[nodiscard]] Int intVal(const std::string& identifier,
                           bool committedValue = false) const {
    if (_invariantGraph->containsVarNode(identifier)) {
      const auto& vNode = _invariantGraph->varNodeConst(identifier);
      assert(vNode.isIntVar());
      if (vNode.varId() != propagation::NULL_ID) {
        return committedValue ? _solver->committedValue(vNode.varId())
                              : _solver->currentValue(vNode.varId());
      }
      assert(vNode.isFixed());
      return vNode.lowerBound();
    }
    if (intPars.contains(identifier)) {
      return intPars.at(identifier);
    }
    throw std::logic_error("Unhandled argument type");
  }

  [[nodiscard]] bool inDomain(const std::string& identifier, Int val) const {
    if (_invariantGraph->containsVarNode(identifier)) {
      const auto& vNode = _invariantGraph->varNodeConst(identifier);
      assert(vNode.isIntVar());
      return vNode.inDomain(val);
    }
    if (intPars.contains(identifier)) {
      return intPars.at(identifier) == val;
    }
    throw std::logic_error("Unhandled argument type");
  }

  [[nodiscard]] propagation::VarViewId totalViolationVarId() const {
    return _invariantGraph->totalViolationVarId();
  }

  [[nodiscard]] Int violation(bool committedValue) const {
    return committedValue ? _solver->committedValue(totalViolationVarId()) : _solver->currentValue(totalViolationVarId());
  }

  [[nodiscard]] std::vector<propagation::VarViewId> getVarIds(
      const std::vector<std::string>& varIdentifiers) const {
    std::vector<propagation::VarViewId> varIds;
    varIds.reserve(varIdentifiers.size());
    for (const std::string& identifier : varIdentifiers) {
      EXPECT_TRUE(_invariantGraph->containsVarNode(identifier));
      const VarNode& vNode = _invariantGraph->varNode(identifier);
      const propagation::VarViewId vId = vNode.varId();
      if (!vNode.isFixed() && vId != propagation::NULL_ID) {
        varIds.emplace_back(vId);
      }
    }
    return varIds;
  }

  [[nodiscard]] std::vector<Int> makeInputVals(
      const std::vector<propagation::VarViewId>& varIds) const {
    std::vector<Int> inputVals;
    inputVals.reserve(varIds.size());
    for (const propagation::VarViewId& vId : varIds) {
      EXPECT_NE(vId, propagation::NULL_ID);
      inputVals.emplace_back(_solver->lowerBound(vId));
    }
    return inputVals;
  }

  Int increaseNextVal(const std::vector<propagation::VarViewId>& varIds,
                      std::vector<Int>& inputVals) const {
    EXPECT_EQ(varIds.size(), inputVals.size());
    for (Int i = static_cast<Int>(inputVals.size() - 1); i >= 0; --i) {
      if (varIds.at(i) == propagation::NULL_ID) {
        continue;
      }
      if (inputVals.at(i) < _solver->upperBound(varIds.at(i))) {
        ++inputVals.at(i);
        return i;
      }
      inputVals.at(i) = _solver->lowerBound(varIds.at(i));
    }
    return -1;
  }

  void setVarVals(const std::vector<propagation::VarViewId>& varIds,
                  const std::vector<Int>& vals) const {
    EXPECT_EQ(varIds.size(), vals.size());
    for (size_t i = 0; i < varIds.size(); ++i) {
      if (varIds.at(i) != propagation::NULL_ID) {
        _solver->setValue(varIds.at(i), vals.at(i));
      }
    }
  }

  std::shared_ptr<IntVar> genIntVar(Int lb, Int ub,
                                    const std::string& identifier = "i") {
    return std::get<std::shared_ptr<IntVar>>(
        _model->addVar(std::make_shared<IntVar>(lb, ub, identifier)));
  }

  std::shared_ptr<IntVar> genIntVar(const std::string& identifier = "i") {
    return genIntVar(defaultLb, defaultUb, identifier);
  }

  std::shared_ptr<IntVar> genIntVar(IntArgState state, const std::string& identifier = "i") {
    return genIntVar(state, defaultLb, defaultUb, identifier);
  }

  std::shared_ptr<IntVar> genIntVar(IntArgState state, Int lb, Int ub, const std::string& identifier = "i") {
    switch (state) {
      case IntArgState::FIXED: {
        const Int val = *rc::gen::inRange<Int>(lb, ub + 1);
        return genIntVar(val, val, identifier);
      }
      case IntArgState::VAR: {
        return genIntVar(lb, ub, identifier);
      }
      default:
        throw std::logic_error("Unhandled argument type");
    }
  }

  void addBoolPar(const std::string& identifier, bool val) {
    assert(!identifier.empty());
    assert(!boolPars.contains(identifier));
    assert(!intPars.contains(identifier));
    boolPars.emplace(identifier, val);
  }

  void addIntPar(const std::string& identifier, Int val) {
    assert(!identifier.empty());
    assert(!boolPars.contains(identifier));
    assert(!intPars.contains(identifier));
    intPars.emplace(identifier, val);
  }

  IntArg addIntArg(IntArgState state, Int lb, Int ub,
                   const std::string& identifier = "i") {
    switch (state) {
      case IntArgState::PAR: {
        const Int val = *rc::gen::inRange<Int>(lb, ub + 1);
        addIntPar(identifier, val);
        auto arg = IntArg{val};
        args.emplace_back(arg);
        return arg;
      }
      case IntArgState::FIXED: {
        const Int val = *rc::gen::inRange<Int>(lb, ub + 1);
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
        throw std::invalid_argument("Invalid IntArgState");
    }
  }

  IntArg addIntArg(IntArgState state, const std::string& identifier = "i") {
    return addIntArg(state, defaultLb, defaultUb, identifier);
  }

  IntArg addIntArg(Int lb, Int ub, const std::string& identifier = "i") {
    return addIntArg(*rc::gen::arbitrary<IntArgState>(), lb, ub, identifier);
  }

  IntArg addIntArg(const std::string& identifier = "i") {
    return addIntArg(defaultLb, defaultUb, identifier);
  }

  [[nodiscard]] std::shared_ptr<IntVarArray> genIntParArray(
      size_t arraySize, Int lb, Int ub,
      const std::string& identifier = "i_arr") {
    auto parArray = std::get<std::shared_ptr<IntVarArray>>(
        _model->addVar(std::make_shared<IntVarArray>(identifier)));
    const std::vector<Int> pars = *rc::gen::container<std::vector<Int>>(
        arraySize, rc::gen::inRange<Int>(lb, ub));
    for (size_t i = 0; i < arraySize; ++i) {
      parArray->append(pars.at(i));
    }
    return parArray;
  }

  std::shared_ptr<IntVarArray> genIntVarArray(
      size_t arraySize, Int lb, Int ub, const std::string& identifier = "i_arr",
      const std::string& varPrefix = "i_") {
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
        default:
          throw std::invalid_argument("Invalid IntArgState");
      }
    }
    return vars;
  }

  std::shared_ptr<IntVarArray> genIntVarArray(
      size_t arraySize, const std::string& identifier = "i_arr",
      const std::string& varPrefix = "i_") {
    return genIntVarArray(arraySize, defaultLb, defaultUb, identifier,
                          varPrefix);
  }

  std::shared_ptr<IntVarArray> genIntParArray(
      size_t arraySize, const std::string& identifier = "i_arr") {
    return genIntParArray(arraySize, defaultLb, defaultUb, identifier);
  }

  std::shared_ptr<BoolVar> genBoolVar(BoolArgState state,
                                      const std::string& identifier = "b") {
    switch (state) {
      case BoolArgState::FIXED_FALSE:
      case BoolArgState::FIXED_TRUE: {
        auto fixedVar = std::make_shared<BoolVar>(
            state == BoolArgState::FIXED_TRUE, identifier);
        _model->addVar(fixedVar);
        return fixedVar;
      }
      case BoolArgState::VAR: {
        auto var = std::make_shared<BoolVar>(identifier);
        _model->addVar(var);
        return var;
      }
      default:
        throw std::invalid_argument("Invalid BoolArgState");
    }
  }

  BoolArg addBoolArg(BoolArgState state, const std::string& identifier = "b") {
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

  BoolArg addBoolArg(const std::string& identifier = "b") {
    return addBoolArg(*rc::gen::arbitrary<BoolArgState>(), identifier);
  }

  std::shared_ptr<BoolVarArray> genBoolParArray(
      size_t arraySize, const std::string& identifier = "b_arr") {
    std::vector<bool> pars = *rc::gen::container<std::vector<bool>>(
        arraySize, rc::gen::arbitrary<bool>());
    auto argArray = std::get<std::shared_ptr<BoolVarArray>>(
        _model->addVar(std::make_shared<BoolVarArray>(identifier)));
    for (size_t i = 0; i < arraySize; ++i) {
      argArray->append(pars.at(i));
    }
    return argArray;
  }

  std::shared_ptr<BoolVarArray> addBoolVarArray(
      size_t arraySize, const std::string& identifier = "b_arr",
      const std::string& varPrefix = "b_") {
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

  std::shared_ptr<BoolVarArray> addBoolVarArray(
      const std::vector<std::string>& identifiers,
      const std::string& identifier = "b_arr") {
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
          addBoolPar(identifiers.at(i), argStates.at(i) == BoolArgState::PAR_TRUE);
          break;
        default:
          vars->append(genBoolVar(argStates.at(i), identifiers.at(i)));
          break;
      }
    }
    args.emplace_back(vars);
    return vars;
  }

  std::shared_ptr<IntVarArray> addIntVarArray(
      size_t arraySize, const std::string& identifier = "i_arr",
      const std::string& varPrefix = "i_") {
    std::vector<IntArgState> argStates =
        *rc::gen::container<std::vector<IntArgState>>(
            arraySize, rc::gen::arbitrary<IntArgState>());
    auto vars = std::get<std::shared_ptr<IntVarArray>>(
        _model->addVar(std::make_shared<IntVarArray>(identifier)));
    for (size_t i = 0; i < arraySize; ++i) {
      const std::string varIdentifier = varPrefix + std::to_string(i);
      switch (argStates.at(i)) {
        case IntArgState::PAR: {
          const Int val = *rc::gen::inRange<Int>(defaultLb, defaultUb + 1);
          vars->append(val);
          addIntPar(varIdentifier, val);
          break;
        }
        default:
          vars->append(genIntVar(argStates.at(i), varIdentifier));
          break;
      }
    }
    args.emplace_back(vars);
    return vars;
  }

  std::shared_ptr<IntVarArray> addIntVarArray(
      const std::vector<std::string>& identifiers,
      const std::string& identifier = "i_arr") {
    std::vector<IntArgState> argStates =
        *rc::gen::container<std::vector<IntArgState>>(
            identifiers.size(), rc::gen::arbitrary<IntArgState>());
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

  Arg genArg(ArgState argState, const std::string& identifierPrefix = "") {
    if (argState.isArray) {
      const size_t arraySize = *rc::gen::inRange<size_t>(0, 4);
      if (std::holds_alternative<BoolArgState>(argState.state)) {
        switch (std::get<BoolArgState>(argState.state)) {
          case BoolArgState::PAR_FALSE:
          case BoolArgState::PAR_TRUE:
            return genBoolParArray(arraySize, identifierPrefix + "b_par_a");
          default:
            return addBoolVarArray(arraySize, identifierPrefix + "b_var_a",
                                   identifierPrefix + "b_var_a_");
        }
      }
      switch (std::get<IntArgState>(argState.state)) {
        case IntArgState::PAR:
          return genIntParArray(arraySize, identifierPrefix + "i_par_a");
        default:
          return genIntVarArray(arraySize, identifierPrefix + "i_var_a",
                                identifierPrefix + "i_var_a_");
      }
    }
    if (std::holds_alternative<BoolArgState>(argState.state)) {
      return addBoolArg(std::get<BoolArgState>(argState.state),
                        identifierPrefix + "b_var");
    }
    return addIntArg(std::get<IntArgState>(argState.state),
                     identifierPrefix + "i_var");
  }

  Arg addArg(Int val) { return args.emplace_back(IntArg{val}); }

  Arg addArg(const std::vector<Int>& parameters,
             const std::string& identifier = "i_par_arr") {
    auto i_par_arr = std::make_shared<IntVarArray>(identifier);
    for (size_t i = 0; i < parameters.size(); ++i) {
      i_par_arr->append(parameters.at(i));
    }
    args.emplace_back(i_par_arr);
    return i_par_arr;
  }

  Arg addArg(const std::vector<bool>& parameters,
             const std::string& identifier = "b_par_arr") {
    auto b_par_arr = std::make_shared<BoolVarArray>(identifier);
    for (size_t i = 0; i < parameters.size(); ++i) {
      b_par_arr->append(parameters.at(i));
    }
    args.emplace_back(b_par_arr);
    return b_par_arr;
  }

  bool randBool() { return binaryDist(gen) == 1; }

  void changeValue(const std::string& identifier, bool committedValue) {
    const auto& var = _model->var(identifier);
    Int lb = 0;
    Int ub = 0;
    if (std::holds_alternative<std::shared_ptr<BoolVar>>(var)) {
      lb = std::get<std::shared_ptr<BoolVar>>(var)->contains(true) ? 0 : 1;
      ub = std::get<std::shared_ptr<BoolVar>>(var)->contains(false) ? 1 : 0;
    }
    if (std::holds_alternative<std::shared_ptr<IntVar>>(var)) {
      lb = std::get<std::shared_ptr<IntVar>>(var)->lowerBound();
      ub = std::get<std::shared_ptr<IntVar>>(var)->upperBound();
    }
    if (lb == ub) {
      return;
    }
    const Int curVal = committedValue
                           ? _solver->committedValue(varId(identifier))
                           : _solver->currentValue(varId(identifier));
    const Int newVal = std::uniform_int_distribution<Int>(lb, ub - 1)(gen);
    _solver->setValue(varId(identifier), curVal != newVal ? newVal : ub);
  }

  void rapidCheck() {
    generate();
    const bool neverSat = neverSatisfied();
    try {
      _invariantGraph->construct();
      _invariantGraph->close();
    } catch (const DomainException&) {
      RC_ASSERT(neverSat);
      return;
    } catch (const InconsistencyException&) {
      RC_ASSERT(neverSat);
      return;
    }
    if (neverSat) {
      RC_ASSERT(!neverSatisfied());
    }
    RC_ASSERT(!neverSat);
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
          _solver->endCommit();
        } else {
          _solver->endProbe();
        }
        RC_ASSERT(isSatisfied(false));
      }
      RC_ASSERT(isSatisfied(true));
    }
  }
};

}  // namespace atlantis::testing
