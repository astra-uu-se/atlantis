#pragma once

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <rapidcheck/gen/Numeric.h>
#include <rapidcheck/gtest.h>

#include <deque>
#include <fznparser/constraint.hpp>
#include <fznparser/model.hpp>
#include <fznparser/types.hpp>
#include <fznparser/variables.hpp>
#include <random>
#include <string>
#include <vector>

#include "atlantis/invariantgraph/fznInvariantGraph.hpp"
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

class FznTestBase : public ::testing::Test {
 public:
  std::shared_ptr<Model> _model;
  std::shared_ptr<FznInvariantGraph> _invariantGraph;
  std::shared_ptr<propagation::Solver> _solver;
  std::string constraintIdentifier;

  void SetUp() override {
    _model = std::make_shared<Model>();
    _solver = std::make_shared<propagation::Solver>();
    _invariantGraph = std::make_shared<FznInvariantGraph>(*_solver, true);
  }

  [[nodiscard]] VarNodeId varNodeId(const std::string& identifier) const {
    return _invariantGraph->varNodeId(identifier);
  }

  [[nodiscard]] propagation::VarViewId varId(
      const std::string& identifier) const {
    return _invariantGraph->varId(identifier);
  }

  void setValue(const std::string& identifier, Int val) const {
    _solver->setValue(varId(identifier), val);
  }

  [[nodiscard]] Int currentValue(const std::string& identifier) const {
    return _solver->currentValue(varId(identifier));
  }

  [[nodiscard]] propagation::VarViewId totalViolationVarId() const {
    return _invariantGraph->totalViolationVarId();
  }

  [[nodiscard]] Int violation() const {
    return _solver->currentValue(totalViolationVarId());
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

  IntArgState genIntArgState() {
    return static_cast<IntArgState>(*rc::gen::inRange<unsigned char>(0, 2));
  }

  std::shared_ptr<fznparser::IntVar> genIntVar(
      Int lb, Int ub, const std::string& identifier = "i") {
    return std::get<std::shared_ptr<fznparser::IntVar>>(
        _model->addVar(std::make_shared<IntVar>(lb, ub, identifier)));
  }

  fznparser::IntArg genIntArg(IntArgState state, Int lb, Int ub,
                              const std::string& identifier = "i") {
    switch (state) {
      case IntArgState::PAR:
        return fznparser::IntArg{*rc::gen::inRange<Int>(lb, ub + 1)};
      case IntArgState::FIXED: {
        const Int val = *rc::gen::inRange<Int>(lb, ub + 1);
        return fznparser::IntArg{genIntVar(val, val, identifier)};
      }
      case IntArgState::VAR:
        return fznparser::IntArg{genIntVar(lb, ub, identifier)};
      default:
        throw std::invalid_argument("Invalid IntArgState");
    }
  }

  fznparser::IntArg genIntArg(Int lb, Int ub,
                              const std::string& identifier = "b") {
    return genIntArg(genIntArgState(), lb, ub, identifier);
  }

  std::shared_ptr<fznparser::IntVarArray> genIntVarArray(
      size_t numVars, Int lb, Int ub, const std::string& identifier = "i_arr",
      const std::string& varPrefix = "i_") {
    auto vars = std::make_shared<fznparser::IntVarArray>(identifier);
    std::vector<unsigned char> argStates =
        *rc::gen::container<std::vector<unsigned char>>(
            numVars, rc::gen::inRange<unsigned char>(0, 3));
    for (size_t i = 0; i < numVars; ++i) {
      const auto state = static_cast<IntArgState>(argStates.at(i));
      switch (state) {
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

  BoolArgState genBoolArgState() {
    return static_cast<BoolArgState>(*rc::gen::inRange<unsigned char>(0, 5));
  }

  std::shared_ptr<fznparser::BoolVar> genBoolVar(
      BoolArgState state, const std::string& identifier = "b") {
    switch (state) {
      case BoolArgState::FIXED_FALSE:
        return std::get<std::shared_ptr<fznparser::BoolVar>>(
            _model->addVar(std::make_shared<BoolVar>(false, identifier)));
      case BoolArgState::FIXED_TRUE:
        return std::get<std::shared_ptr<fznparser::BoolVar>>(
            _model->addVar(std::make_shared<BoolVar>(true, identifier)));
      case BoolArgState::VAR:
        return std::get<std::shared_ptr<fznparser::BoolVar>>(
            _model->addVar(std::make_shared<BoolVar>(identifier)));
      default:
        throw std::invalid_argument("Invalid BoolArgState");
    }
  }

  fznparser::BoolArg genBoolArg(BoolArgState state,
                                const std::string& identifier = "b") {
    switch (state) {
      case BoolArgState::PAR_FALSE:
        return fznparser::BoolArg{false};
      case BoolArgState::PAR_TRUE:
        return fznparser::BoolArg{true};
      default:
        return genBoolVar(state, identifier);
    }
  }

  fznparser::BoolArg genBoolArg(const std::string& identifier = "b") {
    return genBoolArg(genBoolArgState(), identifier);
  }

  std::shared_ptr<fznparser::BoolVarArray> genBoolVarArray(
      size_t numVars, const std::string& identifier = "b_arr",
      const std::string& varPrefix = "b_") {
    std::vector<unsigned char> argStates =
        *rc::gen::container<std::vector<unsigned char>>(
            numVars, rc::gen::inRange<unsigned char>(0, 5));
    auto vars = std::make_shared<fznparser::BoolVarArray>(identifier);
    for (size_t i = 0; i < numVars; ++i) {
      const auto state = static_cast<BoolArgState>(argStates.at(i));
      switch (state) {
        case BoolArgState::PAR_FALSE:
        case BoolArgState::PAR_TRUE:
          vars->append(state == BoolArgState::PAR_TRUE);
          break;
        default:
          vars->append(genBoolVar(state, varPrefix + std::to_string(i)));
          break;
      }
    }
    return vars;
  }
};

}  // namespace atlantis::testing
