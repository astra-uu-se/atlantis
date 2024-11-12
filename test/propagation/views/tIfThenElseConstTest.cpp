#include "../viewTestHelper.hpp"
#include "atlantis/propagation/views/ifThenElseConst.hpp"

namespace atlantis::testing {

using namespace atlantis::propagation;

class IfThenElseConstTest : public ViewTest {
 public:
  Int thenVal{0};
  Int elseVal{0};
  Int condVal{0};
  Int valueLb = std::numeric_limits<Int>::min();
  Int valueUb = std::numeric_limits<Int>::max();
  std::uniform_int_distribution<Int> valueDist;

  void SetUp() override {
    inputVarLb = 0;
    inputVarUb = 1;
    ViewTest::SetUp();
  }

  void generate() {
    valueDist = std::uniform_int_distribution<Int>(valueLb, valueUb);
    thenVal = valueDist(gen);
    elseVal = valueDist(gen);

    _solver->open();
    makeInputVar();
    outputVar = _solver->makeIntView<IfThenElseConst>(
        *_solver, inputVar, thenVal, elseVal, condVal);
    _solver->close();
  }

  Int computeOutput(bool committedValue = false) {
    return (committedValue ? _solver->committedValue(inputVar)
                           : _solver->currentValue(inputVar)) == condVal
               ? thenVal
               : elseVal;
  }
};

TEST_F(IfThenElseConstTest, Bounds) {
  std::vector<std::pair<Int, Int>> bounds{{0, 0}, {0, 1}};
  std::vector<Int> condVals{-1, 0, 1};

  for (const auto& cv : condVals) {
    condVal = cv;
    generate();
    for (const auto& [inputLb, inputUb] : bounds) {
      _solver->updateBounds(VarId(inputVar), inputLb, inputUb, false);

      if (inputLb == condVal && condVal == inputUb) {
        // always then;
        EXPECT_EQ(_solver->lowerBound(outputVar), thenVal);
        EXPECT_EQ(_solver->upperBound(outputVar), thenVal);
      } else if (inputLb <= condVal && condVal <= inputUb) {
        EXPECT_EQ(_solver->lowerBound(outputVar), std::min(thenVal, elseVal));
        EXPECT_EQ(_solver->upperBound(outputVar), std::max(thenVal, elseVal));
      } else {
        // always else:
        EXPECT_EQ(_solver->lowerBound(outputVar), elseVal);
        EXPECT_EQ(_solver->upperBound(outputVar), elseVal);
      }
    }
  }
}

TEST_F(IfThenElseConstTest, Value) {
  inputVarLb = -1;
  inputVarUb = 1;
  generate();

  for (Int inputVal = inputVarLb; inputVal <= inputVarUb; ++inputVal) {
    _solver->setValue(_solver->currentTimestamp(), inputVar, condVal);
    EXPECT_EQ(_solver->currentValue(inputVar), condVal);
    const Int expectedOutput = computeOutput();

    EXPECT_EQ(expectedOutput, _solver->currentValue(outputVar));
  }
}

TEST_F(IfThenElseConstTest, values) {
  generate();
  const std::vector<Int> values{0, 1, 0, 0, 1, 1, 0};

  for (size_t m = 0; m < values.size(); ++m) {
    EXPECT_EQ(_solver->committedValue(outputVar), computeOutput(true));

    for (size_t p = 0; p < values.size(); ++p) {
      const Int curVal = values.at(p);
      _solver->beginMove();
      _solver->setValue(inputVar, curVal);
      _solver->endMove();

      EXPECT_EQ(_solver->currentValue(outputVar), computeOutput());
      if (p == values.size() - 1) {
        _solver->beginMove();
      } else {
        _solver->beginProbe();
      }
      _solver->query(outputVar);
      if (p == values.size() - 1) {
        _solver->endMove();
      } else {
        _solver->endProbe();
      }
      EXPECT_EQ(_solver->currentValue(outputVar), computeOutput());
      EXPECT_EQ(_solver->committedValue(outputVar), computeOutput(true));
    }
  }
}

}  // namespace atlantis::testing
