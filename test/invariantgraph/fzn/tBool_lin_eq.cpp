#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <rapidcheck/gen/Numeric.h>
#include <rapidcheck/gtest.h>

#include <iostream>
#include <string>
#include <vector>

#include "./fznTestBase.hpp"
#include "atlantis/invariantgraph/fzn/bool_lin_eq.hpp"
#include "atlantis/invariantgraph/invariantNodes/boolLinearNode.hpp"

namespace atlantis::testing {

using ::testing::AtLeast;
using ::testing::AtMost;

using namespace atlantis::invariantgraph;
using namespace atlantis::invariantgraph::fzn;

class bool_lin_eqTest : public FznTestBase {
 public:
  std::vector<std::string> inputs{};
  std::vector<Int> coeffs{};
  std::string reified{"reified"};
  Int bound{0};

  [[nodiscard]] Int getFixedLHS() const {
    Int total = 0;
    for (size_t i = 0; i < coeffs.size(); ++i) {
      if (isFixed(inputs.at(i)) && boolVal(inputs.at(i))) {
        total += coeffs.at(i);
      }
    }
    return total;
  }

  [[nodiscard]] bool sameCoeff(Int& coeff) const {
    bool initialized = false;
    for (size_t i = 0; i < coeffs.size(); ++i) {
      if (isFixed(inputs.at(i)) || coeffs.at(i) == 0) {
        continue;
      }
      if (initialized && coeff != std::abs(coeffs.at(i))) {
        return false;
      }
      initialized = true;
      coeff = std::abs(coeffs.at(i));
    }
    return initialized;
  }

  [[nodiscard]] std::pair<Int, Int> getBounds() const {
    Int lb = 0;
    Int ub = 0;
    for (size_t i = 0; i < coeffs.size(); ++i) {
      if (coeffs.at(i) == 0) {
        continue;
      }
      if (isFixed(inputs.at(i))) {
        lb += boolVal(inputs.at(i)) ? coeffs.at(i) : 0;
        ub += boolVal(inputs.at(i)) ? coeffs.at(i) : 0;
      } else {
        lb += std::min<Int>(0, coeffs.at(i));
        ub += std::max<Int>(0, coeffs.at(i));
      }
    }
    return {lb, ub};
  }

  [[nodiscard]] bool isSatisfied(bool committedValue) const override {
    Int sum = 0;
    for (size_t i = 0; i < coeffs.size(); ++i) {
      if (coeffs.at(i) != 0) {
        sum += boolVal(inputs.at(i), committedValue) ? coeffs.at(i) : 0;
      }
    }
    const bool actual = boolVal(reified, committedValue);
    const bool expected = sum == bound;

    if (isFixed(reified)) {
      const bool isSolution = violation(committedValue) == 0;
      return isSolution ? expected == actual : expected != actual;
    }
    return expected == actual;
  }

  [[nodiscard]] bool alwaysSatisfied() const override {
    const auto [lb, ub] = getBounds();
    const bool alwaysSat = lb == ub && lb == bound;
    if (alwaysSat) {
      return isFixedTo(reified, bool{true});
    }
    const bool alwaysUnsat = bound < lb || ub < bound;
    if (alwaysUnsat) {
      return isFixedTo(reified, bool{false});
    }
    const Int total = bound - getFixedLHS();
    Int coeff;
    if (sameCoeff(coeff) && total % coeff != 0) {
      return isFixedTo(reified, bool{false});
    }

    return false;
  }

  [[nodiscard]] bool neverSatisfied() const override {
    if (!isFixed(reified)) {
      return false;
    }
    const auto [lb, ub] = getBounds();
    const bool alwaysSat = lb == ub && lb == bound;
    if (alwaysSat) {
      return isFixedTo(reified, bool{false});
    }

    const bool alwaysUnsat = bound < lb || ub < bound;

    if (alwaysUnsat) {
      return isFixedTo(reified, bool{true});
    }

    const Int total = bound - getFixedLHS();
    Int coeff;
    if (sameCoeff(coeff) && total % coeff != 0) {
      return isFixedTo(reified, bool{true});
    }

    return false;
  }

  void generate() override {
    const size_t size = *rc::gen::inRange<size_t>(0, 4);
    coeffs =
        *rc::gen::container<std::vector<Int>>(size, rc::gen::inRange(-2, 2));
    addArg(coeffs);
    inputs.reserve(size);
    for (size_t i = 0; i < size; ++i) {
      inputs.emplace_back("b_" + std::to_string(i));
    }
    addBoolVarArray(inputs);

    Int lb = -1;
    Int ub = 2;
    for (const Int c : coeffs) {
      ub += std::max<Int>(0, c);
      lb += std::min<Int>(0, c);
    }

    bound = *rc::gen::inRange<Int>(lb, ub);
    addArg(bound);

    constraintIdentifier = "bool_lin_eq";
    addBoolPar(reified, true);
    generateConstraint();
  }

  [[nodiscard]] bool canMove() const override {
    return std::ranges::any_of(inputs, [&](const std::string& input) {
      return varId(input) != propagation::NULL_ID;
    });
  }

  void move(bool committedValue) override {
    std::unordered_set<InvariantNodeId, InvariantNodeIdHash>
        implicitConstraints;
    std::vector<bool> hasImplicitConstraints(inputs.size(), false);
    implicitConstraints.reserve(inputs.size());
    for (size_t i = 0; i < inputs.size(); ++i) {
      if (!isFixed(inputs.at(i))) {
        const auto& defNodes = varNodeConst(inputs.at(i)).definingNodes();
        if (!defNodes.empty()) {
          RC_ASSERT(defNodes.size() == size_t{1});
          const InvariantNodeId implId = *defNodes.begin();
          RC_ASSERT(implId.isImplicitConstraint());
          implicitConstraints.emplace(implId);
          hasImplicitConstraints.at(i) = true;
        }
      }
    }

    for (size_t i = 0; i < inputs.size(); ++i) {
      if (!hasImplicitConstraints.at(i) &&
          varId(inputs.at(i)) != propagation::NULL_ID && randBool()) {
        changeValue(inputs.at(i), committedValue);
      }
    }

    for (const InvariantNodeId implId : implicitConstraints) {
      auto implNode = _solverMapping->neighborhood(implId);
      RC_ASSERT(implNode != nullptr);
      implNode->randomMove(*_randomProvider, *_assignment);
    }
  }

  void query() override {
    if (varId(reified) != propagation::NULL_ID) {
      _solver->query(varId(reified));
    } else if (totalViolationVarId() != propagation::NULL_ID) {
      _solver->query(totalViolationVarId());
    }
  }
};

RC_GTEST_FIXTURE_PROP(bool_lin_eqTest, RapidCheck, ()) { rapidCheck(); }

TEST_F(bool_lin_eqTest, SupportsVariableBoundOutput) {
  coeffs = {1, 1, 1};
  addArg(coeffs);

  inputs = {"b_0", "b_1", "b_2"};
  addBoolVarArray({BoolArgState::VAR, BoolArgState::VAR, BoolArgState::VAR},
                  inputs);

  const std::string sum{"sum"};
  addIntArg(IntArgState::VAR, 0, 3, sum);

  constraintIdentifier = "bool_lin_eq";
  generateConstraint();

  ASSERT_NE(varNodeId(sum), NULL_NODE_ID);
  ASSERT_EQ(varNodeConst(sum).definingNodes().size(), 1U);
  const auto invId = *varNodeConst(sum).definingNodes().begin();
  EXPECT_NE(dynamic_cast<const BoolLinearNode*>(
                &_invariantGraph->invariantNodeConst(invId)),
            nullptr);
}

}  // namespace atlantis::testing
