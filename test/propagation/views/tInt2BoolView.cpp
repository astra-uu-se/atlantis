#include "../viewTestHelper.hpp"
#include "atlantis/propagation/views/int2BoolView.hpp"

namespace atlantis::testing {

using namespace atlantis::propagation;

class Int2BoolViewTest : public ViewTest {
 public:
  void SetUp() override {
    inputVarLb = -1;
    inputVarUb = 2;
    ViewTest::SetUp();
  }

  void generate() {
    _solver->open();
    makeInputVar();
    outputVar = _solver->makeIntView<Int2BoolView>(*_solver, inputVar);
    _solver->close();
  }

  Int computeOutput(bool committedValue = false) {
    return (committedValue ? _solver->committedValue(inputVar)
                           : _solver->currentValue(inputVar)) <= 0
               ? 1
               : 0;
  }
};

TEST_F(Int2BoolViewTest, bounds) {
  std::vector<std::pair<Int, Int>> bounds{
      {-100, -100}, {0, 0}, {0, 1}, {1, 1}, {1000, 1000}};
  std::vector<std::pair<Int, Int>> expected{
      {1, 1}, {1, 1}, {0, 1}, {0, 0}, {0, 0}};
  EXPECT_EQ(bounds.size(), expected.size());
  generate();

  for (size_t i = 0; i < bounds.size(); ++i) {
    const auto& [inputLb, inputUb] = bounds.at(i);
    EXPECT_LE(inputLb, inputUb);

    const auto& [expectedLb, expectedUb] = expected.at(i);
    EXPECT_LE(expectedLb, expectedUb);

    _solver->updateBounds(VarId(inputVar), inputLb, inputUb, false);

    EXPECT_EQ(_solver->lowerBound(outputVar), expectedLb);
    EXPECT_EQ(_solver->upperBound(outputVar), expectedUb);
  }
}

TEST_F(Int2BoolViewTest, values) {
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
