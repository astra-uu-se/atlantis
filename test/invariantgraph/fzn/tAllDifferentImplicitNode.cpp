#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <deque>
#include <string>
#include <vector>

#include "./fznTestBase.hpp"
#include "atlantis/invariantgraph/fzn/allDifferentImplicitNode.hpp"
#include "atlantis/invariantgraph/fznInvariantGraph.hpp"

namespace atlantis::testing {

using ::testing::AtLeast;
using ::testing::AtMost;

using namespace atlantis::invariantgraph;
using namespace atlantis::invariantgraph::fzn;

class FznAllDifferentImplicitNodeTest : public FznTestBase {
 public:
  Int numInputs = 4;
  std::vector<std::string> inputIdentifiers{};

  void SetUp() override {
    FznTestBase::SetUp();
    constraintIdentifier = "fzn_all_different_int";

    std::vector<Arg> args;
    args.reserve(1);

    inputIdentifiers.reserve(numInputs);
    IntVarArray inputsArg("inputs");
    EXPECT_TRUE(numInputs % 2 == 0);
    const Int ub = numInputs / 2;
    const Int lb = -ub;
    for (Int i = 0; i < numInputs; ++i) {
      inputIdentifiers.emplace_back("input" + std::to_string(i));
      _model->addVar(IntVar(lb, ub, inputIdentifiers.back()));
      inputsArg.append(std::get<IntVar>(_model->var(inputIdentifiers.back())));
    }
    args.emplace_back(inputsArg);

    _model->addConstraint(Constraint(constraintIdentifier, std::move(args)));
  }
};

TEST_F(FznAllDifferentImplicitNodeTest, construction) {
  EXPECT_EQ(_model->constraints().size(), 1);
  const InvariantNodeId implicitNodeId = _invariantGraph->nextImplicitNodeId();
  EXPECT_FALSE(_invariantGraph->containsImplicitConstraintNode(implicitNodeId));
  EXPECT_TRUE(makeAllDifferentImplicitNode(*_invariantGraph,
                                           _model->constraints().front()));
  EXPECT_TRUE(_invariantGraph->containsImplicitConstraintNode(implicitNodeId));
  for (const auto& identifier : inputIdentifiers) {
    EXPECT_TRUE(_invariantGraph->containsVarNode(identifier));
    EXPECT_NE(_invariantGraph->varNodeId(identifier), NULL_NODE_ID);
  }
}

}  // namespace atlantis::testing
