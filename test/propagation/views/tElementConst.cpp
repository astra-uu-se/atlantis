#include "../viewTestHelper.hpp"
#include "atlantis/propagation/views/elementConst.hpp"

namespace atlantis::testing {

using namespace atlantis::propagation;

class ElementConstTest : public ViewTest {
 public:
  Int numValues = 4;
  Int offset{1};

  Int valueLb = std::numeric_limits<Int>::min();
  Int valueUb = std::numeric_limits<Int>::max();
  std::vector<Int> values;
  std::uniform_int_distribution<Int> valueDist;

  Int indexLb() const { return offset; }
  Int indexUb() const { return offset + numValues - 1; }

  void SetUp() override {
    inputVarLb = indexLb();
    inputVarUb = indexUb();
    ViewTest::SetUp();
  }

  void TearDown() override {
    ViewTest::TearDown();
    values.clear();
  }

  Int toZeroIndex(Int index) const {
    const Int zeroIndex = index - offset;
    assert(0 <= zeroIndex);
    assert(zeroIndex < static_cast<Int>(values.size()));
    return zeroIndex;
  }

  Int computeOutput(bool committedValue = false) {
    return values.at(toZeroIndex(committedValue
                                     ? _solver->committedValue(inputVar)
                                     : _solver->currentValue(inputVar)));
  }

  void generate() {
    values.resize(numValues);
    valueDist = std::uniform_int_distribution<Int>(valueLb, valueUb);
    for (Int& value : values) {
      value = valueDist(gen);
    }

    inputVarLb = indexLb();
    inputVarUb = indexUb();
    inputVarDist = std::uniform_int_distribution<Int>(inputVarLb, inputVarUb);

    _solver->open();
    makeInputVar();
    outputVar = _solver->makeIntView<ElementConst>(
        *_solver, inputVar, std::vector<Int>(values), offset);
    _solver->close();
  }
};

TEST_F(ElementConstTest, bounds) {
  std::vector<Int> offsets = {-100, 0, 1, 100};
  for (const Int o : offsets) {
    offset = o;
    generate();

    for (Int minIndex = indexLb(); minIndex <= indexUb(); ++minIndex) {
      for (Int maxIndex = indexUb(); maxIndex >= minIndex; --maxIndex) {
        _solver->updateBounds(VarId(inputVar), minIndex, maxIndex, false);

        Int minVal = std::numeric_limits<Int>::max();
        Int maxVal = std::numeric_limits<Int>::min();

        for (Int index = minIndex; index <= maxIndex; ++index) {
          minVal = std::min(minVal, values.at(toZeroIndex(index)));
          maxVal = std::max(maxVal, values.at(toZeroIndex(index)));
        }
        EXPECT_EQ(minVal, _solver->lowerBound(outputVar));
        EXPECT_EQ(maxVal, _solver->upperBound(outputVar));
      }
    }
  }
}

RC_GTEST_FIXTURE_PROP(ElementConstTest, rapidcheck, ()) {
  numValues = *rc::gen::inRange(1, 100);
  offset = *rc::gen::inRange(std::numeric_limits<Int>::min() + numValues,
                             std::numeric_limits<Int>::max() - numValues);

  generate();

  const size_t numCommits = 3;
  const size_t numProbes = 3;

  for (size_t c = 0; c < numCommits; ++c) {
    for (size_t p = 0; p <= numProbes; ++p) {
      _solver->beginMove();
      _solver->setValue(inputVar, inputVarDist(gen));
      _solver->endMove();

      EXPECT_EQ(_solver->currentValue(outputVar), computeOutput());

      if (p == numProbes) {
        _solver->beginCommit();
      } else {
        _solver->beginProbe();
      }
      _solver->query(outputVar);
      if (p == numProbes) {
        _solver->endCommit();
      } else {
        _solver->endProbe();
      }
      RC_ASSERT(_solver->currentValue(outputVar) == computeOutput());
      RC_ASSERT(_solver->committedValue(outputVar) == computeOutput(true));
    }
    RC_ASSERT(_solver->committedValue(outputVar) == computeOutput(true));
  }
}

}  // namespace atlantis::testing
