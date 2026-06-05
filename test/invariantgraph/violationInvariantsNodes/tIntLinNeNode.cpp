#include <gmock/gmock.h>

#include "../nodeTestBase.hpp"
#include "atlantis/invariantgraph/violationInvariantNodes/intLinRelNode.hpp"

namespace atlantis::testing {

using namespace atlantis::invariantgraph;
using ::testing::ContainerEq;
using ::testing::Contains;

class IntLinNeNodeTestFixture : public NodeTestBase<IntLinRelNode> {
 protected:
  size_t numInputs{3};
  std::vector<Int> coeffs;
  std::vector<Var> inputVars;
  Var reifiedVar{"reified", std::vector<Int>{}, false};

  Int bound = 1;

  [[nodiscard]] bool isViolating(const bool isRegistered = false) const {
    if (isRegistered) {
      Int sum = 0;
      for (size_t i = 0; i < coeffs.size(); ++i) {
        if (coeffs.at(i) == 0) {
          continue;
        }
        if (varNodeConst(inputVars.at(i)).isFixed()) {
          sum += varNodeConst(inputVars.at(i)).lowerBound() * coeffs.at(i);
        } else {
          sum += _solver->currentValue(varId(inputVars.at(i))) * coeffs.at(i);
        }
      }
      return sum == bound;
    }
    Int sum = 0;
    for (size_t i = 0; i < coeffs.size(); ++i) {
      if (coeffs.at(i) == 0) {
        continue;
      }
      EXPECT_TRUE(varNodeConst(inputVars.at(i)).isFixed());
      sum += varNodeConst(inputVars.at(i)).lowerBound() * coeffs.at(i);
    }
    return sum == bound;
  }

  void SetUp() {
    NodeTestBase::SetUp();
    inputVars.reserve(numInputs);
    coeffs.reserve(numInputs);
    const Int lb = -2;
    const Int ub = 2;
    for (Int i = 0; i < static_cast<Int>(numInputs); ++i) {
      inputVars.emplace_back("input_" + std::to_string(i));
      if (shouldBeSubsumed()) {
        const Int val = i % 3 == 0 ? lb : ub;
        retrieveIntVarNode(val, val, inputVars.back());
      } else {
        retrieveIntVarNode(lb, ub, inputVars.back());
      }
      coeffs.emplace_back((i + 1) * (i % 2 == 0 ? -1 : 1));
    }

    if (isReified()) {
      retrieveBoolVarNode(reifiedVar);
      createInvariantNode(*_invariantGraph, std::vector<Int>(coeffs),
                          varNodeIds(inputVars), RelationType::REL_TYPE_NE,
                          bound, varNodeId(reifiedVar));
    } else {
      createInvariantNode(*_invariantGraph, std::vector<Int>(coeffs),
                          varNodeIds(inputVars), RelationType::REL_TYPE_NE,
                          bound, shouldHold());
    }
  }
};

TEST_P(IntLinNeNodeTestFixture, updateState) {
  EXPECT_EQ(invNode().state(), InvariantNodeState::ACTIVE);
  invNode().updateState();
  if (shouldBeSubsumed()) {
    EXPECT_EQ(invNode().state(), InvariantNodeState::SUBSUMED);
    const Int expected = isViolating();
    if (isReified()) {
      EXPECT_TRUE(varNode(reifiedVar).isFixed());
      const Int actual = varNode(reifiedVar).lowerBound();
      EXPECT_EQ(expected, actual);
    } else if (shouldHold()) {
      EXPECT_FALSE(expected);
    } else {
      EXPECT_TRUE(expected);
    }
  } else {
    EXPECT_NE(invNode().state(), InvariantNodeState::SUBSUMED);
    if (isReified()) {
      EXPECT_FALSE(varNode(reifiedVar).isFixed());
    }
  }
}

TEST_P(IntLinNeNodeTestFixture, propagation) {
  _invariantGraph->close();
  _solverMapping =
      std::make_shared<SolverMapping>(_invariantGraph->construct(*_solver));

  if (shouldBeSubsumed()) {
    const bool expected = isViolating();
    if (isReified()) {
      EXPECT_TRUE(varNode(reifiedVar).isFixed());
      const bool actual = varNode(reifiedVar).inDomain(false);
      EXPECT_EQ(expected, actual);
    }
    if (shouldHold()) {
      EXPECT_FALSE(expected);
    }
    if (shouldFail()) {
      EXPECT_TRUE(expected);
    }
    return;
  }

  std::vector<propagation::VarViewId> inputVarIds;
  for (const auto& var : inputVars) {
    if (!varNode(var).isFixed()) {
      EXPECT_NE(varId(var), propagation::NULL_ID);
      inputVarIds.emplace_back(varId(var));
    }
  }

  EXPECT_FALSE(inputVarIds.empty());

  const propagation::VarViewId violVarId =
      isReified() ? varId(reifiedVar) : _solverMapping->totalViolationId();

  EXPECT_NE(violVarId, propagation::NULL_ID);

  std::vector<Int> inputVals = makeInputVals(inputVarIds);

  while (increaseNextVal(inputVarIds, inputVals) >= 0) {
    _solver->beginMove();
    setVarVals(inputVarIds, inputVals);
    _solver->endMove();

    _solver->beginProbe();
    _solver->query(violVarId);
    _solver->endProbe();

    expectVarVals(inputVarIds, inputVals);

    const bool actual = _solver->currentValue(violVarId) > 0;
    const bool expected = isViolating(true);

    if (!shouldFail()) {
      EXPECT_EQ(actual, expected);
    } else {
      EXPECT_NE(actual, expected);
    }
  }
}

INSTANTIATE_TEST_CASE_P(
    IntLinNeNodeTest, IntLinNeNodeTestFixture,
    ::testing::Values(ParamData{ViolationInvariantType::CONSTANT_TRUE},
                      ParamData{ViolationInvariantType::CONSTANT_FALSE},
                      ParamData{ViolationInvariantType::REIFIED},
                      ParamData{InvariantNodeAction::SUBSUME}));

}  // namespace atlantis::testing
