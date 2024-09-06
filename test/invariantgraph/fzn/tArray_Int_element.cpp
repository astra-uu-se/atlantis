#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <deque>
#include <string>
#include <vector>

#include "./fznTestBase.hpp"
#include "atlantis/invariantgraph/fzn/array_int_element.hpp"
#include "atlantis/invariantgraph/fznInvariantGraph.hpp"

namespace atlantis::testing {

using ::testing::AtLeast;
using ::testing::AtMost;

using namespace fznparser;
using namespace atlantis::invariantgraph;
using namespace atlantis::invariantgraph::fzn;

class array_int_elementTest : public FznTestBase {
 public:
  std::string idxIdentifier{};
  std::string outputIdentifier{};
  std::vector<Int> inputVals;
  Int offsetIdx = 1;

  Int numInputs = 4;

  void SetUp() override {
    FznTestBase::SetUp();
    constraintIdentifier = "array_int_element_offset";

    idxIdentifier = "idx";
    _model->addVar(std::make_shared<IntVar>(offsetIdx, numInputs + offsetIdx,
                                            idxIdentifier));
    outputIdentifier = "output";
    _model->addVar(std::make_shared<IntVar>(
        -2, 2, outputIdentifier,
        std::vector<Annotation>{Annotation("is_defined_var")}));

    std::vector<Arg> args;
    args.reserve(3);
    args.emplace_back(
        std::get<std::shared_ptr<IntVar>>(_model->var(idxIdentifier)));

    inputVals.reserve(numInputs);
    auto inputsArg = std::make_shared<IntVarArray>("inputs");
    for (Int i = 0; i < numInputs; ++i) {
      inputVals.emplace_back(i - 2);
      inputsArg->append(inputVals.back());
    }
    args.emplace_back(inputsArg);

    args.emplace_back(
        std::get<std::shared_ptr<IntVar>>(_model->var(outputIdentifier)));
    args.emplace_back(IntArg(offsetIdx));

    _model->addConstraint(Constraint(constraintIdentifier, std::move(args)));
  }
};

TEST_F(array_int_elementTest, construction) {
  EXPECT_EQ(_model->constraints().size(), 1);
  EXPECT_TRUE(
      array_int_element(*_invariantGraph, _model->constraints().front()));
  EXPECT_NE(_invariantGraph->varNodeId(idxIdentifier), NULL_NODE_ID);
  EXPECT_NE(_invariantGraph->varNodeId(outputIdentifier), NULL_NODE_ID);
}

TEST_F(array_int_elementTest, propagation) {
  array_int_element(*_invariantGraph, _model->constraints().front());
  _invariantGraph->construct();
  _invariantGraph->close();

  for (Int i = 0; i < numInputs; ++i) {
    _solver->beginMove();
    setValue(idxIdentifier, i + offsetIdx);
    _solver->endMove();

    _solver->beginProbe();
    _solver->query(varId(outputIdentifier));
    _solver->endProbe();

    const Int actual = currentValue(outputIdentifier);
    const Int expected = inputVals.at(i);

    EXPECT_EQ(actual, expected);
  }
}

}  // namespace atlantis::testing
