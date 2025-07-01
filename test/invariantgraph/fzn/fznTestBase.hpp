#pragma once

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <rapidcheck/gtest.h>

#include <fznparser/constraint.hpp>
#include <fznparser/model.hpp>
#include <fznparser/variables.hpp>
#include <random>
#include <string>
#include <vector>

#include "atlantis/invariantgraph/fznInvariantGraph.hpp"
#include "atlantis/invariantgraph/varNode.hpp"
#include "atlantis/propagation/solver.hpp"
#include "atlantis/search/assignment.hpp"
#include "atlantis/search/neighborhoods/neighborhoodCombinator.hpp"
#include "atlantis/search/randomProvider.hpp"

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

inline std::ostream& operator<<(std::ostream& os, BoolArgState state) {
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

inline std::ostream& operator<<(std::ostream& os, IntArgState state) {
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

namespace rc {
using namespace atlantis::testing;
template <>
struct Arbitrary<BoolArgState> {
  static Gen<BoolArgState> arbitrary() {
    return gen::element<BoolArgState>(
        BoolArgState::PAR_FALSE, BoolArgState::PAR_TRUE,
        BoolArgState::FIXED_FALSE, BoolArgState::FIXED_TRUE, BoolArgState::VAR);
  }
};
template <>
struct Arbitrary<IntArgState> {
  static Gen<IntArgState> arbitrary() {
    return gen::element<IntArgState>(IntArgState::PAR, IntArgState::FIXED,
                                     IntArgState::VAR);
  }
};
}  // namespace rc

namespace atlantis::testing {

std::string to_string(bool v);

class FznTestBase : public ::testing::Test {
 public:
  std::shared_ptr<Model> _model;
  std::shared_ptr<FznInvariantGraph> _invariantGraph;
  std::shared_ptr<propagation::Solver> _solver;
  std::shared_ptr<search::neighborhoods::NeighborhoodCombinator> _neighborhood;
  std::shared_ptr<search::Assignment> _assignment;
  std::shared_ptr<search::RandomProvider> _randomProvider;
  std::string constraintIdentifier;
  std::vector<Annotation> annotations{};
  std::vector<Arg> args;
  std::mt19937 gen;
  std::default_random_engine rng;
  std::uniform_int_distribution<unsigned char> binaryDist;
  std::unordered_map<std::string, Int> intPars;
  std::unordered_map<std::string, bool> boolPars;
  std::unordered_map<std::string, std::vector<Int>> intSetPars;

  const Int defaultLb = -3;
  const Int defaultUb = 3;

  void SetUp() override;

  void generateConstraint();

  virtual void generate() = 0;
  [[nodiscard]] virtual bool isSatisfied(bool committedValue) const = 0;
  [[nodiscard]] virtual bool alwaysSatisfied() const { return false; };
  [[nodiscard]] virtual bool neverSatisfied() const { return false; };
  virtual bool canMove() const = 0;
  virtual void move(bool committedValue) = 0;
  virtual void query() = 0;

  [[nodiscard]] VarNode& varNode(const std::string& identifier);

  [[nodiscard]] const VarNode& varNodeConst(
      const std::string& identifier) const;

  [[nodiscard]] VarNodeId varNodeId(const std::string& identifier) const;

  [[nodiscard]] propagation::VarViewId varId(
      const std::string& identifier) const;

  void setValue(const std::string& identifier, Int val) const;

  [[nodiscard]] Int currentValue(const std::string& identifier) const;

  [[nodiscard]] Int lowerBound(const std::string& identifier) const;

  [[nodiscard]] Int upperBound(const std::string& identifier) const;

  [[nodiscard]] bool isFixed(const std::string& identifier) const;

  [[nodiscard]] bool boolVal(const std::string& identifier,
                             bool committedValue = false) const;

  [[nodiscard]] bool inDomain(const std::string& identifier, bool val) const;

  [[nodiscard]] bool isFixedTo(const std::string& identifier, bool val) const;

  [[nodiscard]] bool isFixedTo(const std::string& identifier, Int val) const;

  [[nodiscard]] Int intVal(const std::string& identifier,
                           bool committedValue = false) const;

  [[nodiscard]] bool inDomain(const std::string& identifier, Int val) const;

  [[nodiscard]] const std::vector<Int>& intSetVal(
      const std::string& identifier) const;

  [[nodiscard]] propagation::VarViewId totalViolationVarId() const;

  [[nodiscard]] Int violation(bool committedValue) const;

  std::shared_ptr<IntVar> genIntVar(Int lb, Int ub,
                                    const std::string& identifier = "i");

  std::shared_ptr<IntVar> genIntVar(const std::vector<Int>& dom,
                                    const std::string& identifier = "i");

  std::shared_ptr<IntVar> genIntVar(const std::string& identifier = "i");

  std::shared_ptr<IntVar> genIntVar(IntArgState state,
                                    const std::string& identifier = "i");

  std::shared_ptr<IntVar> genIntVar(IntArgState state, Int lb, Int ub,
                                    const std::string& identifier = "i");

  void addBoolPar(const std::string& identifier, bool val);

  void addIntPar(const std::string& identifier, Int val);

  void addIntSetPar(const std::string& identifier, std::vector<Int>&& val);

  IntArg addIntArg(IntArgState state, Int lb, Int ub,
                   const std::string& identifier = "i");

  IntArg addIntArg(IntArgState state, const std::vector<Int>& dom,
                   const std::string& identifier = "i");

  IntArg addIntArg(IntArgState state, const std::string& identifier = "i");

  IntArg addIntArg(Int lb, Int ub, const std::string& identifier = "i");

  IntArg addIntArg(const std::string& identifier = "i");

  [[nodiscard]] std::shared_ptr<IntVarArray> genIntParArray(
      size_t arraySize, Int lb, Int ub,
      const std::string& identifier = "i_arr");

  std::shared_ptr<IntVarArray> genIntVarArray(
      size_t arraySize, Int lb, Int ub, const std::string& identifier = "i_arr",
      const std::string& varPrefix = "i_");

  std::shared_ptr<IntVarArray> genIntVarArray(
      size_t arraySize, const std::string& identifier = "i_arr",
      const std::string& varPrefix = "i_");

  std::shared_ptr<IntVarArray> genIntParArray(
      size_t arraySize, const std::string& identifier = "i_arr");

  std::shared_ptr<BoolVar> genBoolVar(BoolArgState state,
                                      const std::string& identifier = "b");
  std::vector<Int> genDomain(size_t size) const;

  std::vector<Int> genDomain(IntArgState state) const;
  std::vector<Int> genDomain() const;

  BoolArg addBoolArg(BoolArgState state, const std::string& identifier = "b");

  BoolArg addBoolArg(const std::string& identifier = "b");

  std::shared_ptr<BoolVarArray> genBoolParArray(
      size_t arraySize, const std::string& identifier = "b_arr");

  std::shared_ptr<BoolVarArray> addBoolVarArray(
      size_t arraySize, const std::string& identifier = "b_arr",
      const std::string& varPrefix = "b_");

  std::shared_ptr<BoolVarArray> addBoolVarArray(
      const std::vector<std::string>& identifiers,
      const std::string& identifier = "b_arr");

  std::shared_ptr<IntVarArray> addIntVarArray(
      const std::vector<IntArgState>& argStates,
      const std::vector<std::pair<Int, Int>>& domains,
      const std::vector<std::string>& identifiers,
      const std::string& identifier = "i_arr");

  std::shared_ptr<IntVarArray> addIntVarArray(
      const std::vector<IntArgState>& argStates,
      const std::vector<std::string>& identifiers,
      const std::string& identifier = "i_arr");

  std::shared_ptr<IntVarArray> addIntVarArray(
      const std::vector<std::string>& identifiers,
      const std::string& identifier = "i_arr");

  std::shared_ptr<IntVarArray> addIntVarArray(
      size_t arraySize, const std::string& identifier = "i_arr",
      const std::string& varPrefix = "i_");

  Arg addArg(Int val);

  Arg addArg(const std::vector<Int>& parameters,
             const std::string& identifier = "i_par_arr");

  Arg addArg(const std::vector<bool>& parameters,
             const std::string& identifier = "b_par_arr");

  Arg addIntSetArg(Int lb, Int ub, const std::string& identifier = "i_set");

  Arg addIntSetArg(const std::vector<Int>& elements,
                   const std::string& identifier = "i_set");

  Arg addIntSetArg(const std::string& identifier = "i_set");

  bool randBool();

  void changeValue(const std::string& identifier, bool committedValue);

  void rapidCheck(bool reachesFixpoint = true);
};

}  // namespace atlantis::testing
