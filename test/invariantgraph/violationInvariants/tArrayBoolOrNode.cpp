#include <gmock/gmock.h>

#include "../nodeTestBase.hpp"
#include "invariantgraph/violationInvariantNodes/arrayBoolOrNode.hpp"
#include "propagation/solver.hpp"

namespace atlantis::testing {

using namespace atlantis::invariantgraph;

using ::testing::ContainerEq;

static bool isViolating(const std::vector<Int>& values) {
  for (const Int val : values) {
    if (val == 0) {
      return false;
    }
  }
  return true;
}

template <ViolationInvariantType Type>
class AbstractArrayBoolOrNodeTest : public NodeTestBase<ArrayBoolOrNode> {
 public:
  VarNodeId a;
  VarNodeId b;
  VarNodeId r;

  void SetUp() override {
    NodeTestBase::SetUp();
    a = createBoolVar("a");
    b = createBoolVar("b");
    r = createBoolVar("r");

    fznparser::BoolVarArray inputs("");
    inputs.append(boolVar(a));
    inputs.append(boolVar(b));

    if constexpr (Type == ViolationInvariantType::REIFIED) {
      _model->addConstraint(std::move(fznparser::Constraint(
          "array_bool_or", std::vector<fznparser::Arg>{
                               inputs, fznparser::BoolArg{boolVar(r)}})));

    } else {
      if constexpr (Type == ViolationInvariantType::NORMAL) {
        _model->addConstraint(std::move(fznparser::Constraint(
            "array_bool_or",
            std::vector<fznparser::Arg>{inputs, fznparser::BoolArg{true}})));
      } else if constexpr (Type == ViolationInvariantType::CONSTANT_FALSE) {
        _model->addConstraint(std::move(fznparser::Constraint(
            "array_bool_or",
            std::vector<fznparser::Arg>{inputs, fznparser::BoolArg{false}})));
      } else {
        _model->addConstraint(std::move(fznparser::Constraint(
            "array_bool_or",
            std::vector<fznparser::Arg>{inputs, fznparser::BoolArg{true}})));
      }
    }

    makeInvNode(_model->constraints().front());
  }

  void construction() {
    expectInputTo(invNode());
    expectOutputOf(invNode());

    EXPECT_EQ(invNode().staticInputVarNodeIds().size(), 2);
    EXPECT_EQ(invNode().dynamicInputVarNodeIds().size(), 0);
    std::vector<VarNodeId> expectedVars{a, b};
    EXPECT_EQ(invNode().staticInputVarNodeIds(), expectedVars);
    EXPECT_THAT(expectedVars, ContainerEq(invNode().staticInputVarNodeIds()));
    if constexpr (Type != ViolationInvariantType::REIFIED) {
      EXPECT_FALSE(invNode().isReified());
      EXPECT_EQ(invNode().reifiedViolationNodeId(), NULL_NODE_ID);
    } else {
      EXPECT_TRUE(invNode().isReified());
      EXPECT_NE(invNode().reifiedViolationNodeId(), NULL_NODE_ID);
      EXPECT_EQ(invNode().reifiedViolationNodeId(), r);
    }
  }

  void application() {
    propagation::Solver solver;
    solver.open();
    addInputVarsToSolver(solver);
    for (const auto& outputVarNodeId : invNode().outputVarNodeIds()) {
      EXPECT_EQ(varId(outputVarNodeId), propagation::NULL_ID);
    }
    invNode().registerOutputVars(*_invariantGraph, solver);
    for (const auto& outputVarNodeId : invNode().outputVarNodeIds()) {
      EXPECT_NE(varId(outputVarNodeId), propagation::NULL_ID);
    }
    invNode().registerNode(*_invariantGraph, solver);
    solver.close();

    // a and b
    EXPECT_EQ(solver.searchVars().size(), 2);

    // a, b and r
    EXPECT_EQ(solver.numVars(), 3);

    // minSparse
    EXPECT_EQ(solver.numInvariants(), 1);
  }

  void propagation() {
    propagation::Solver solver;
    solver.open();
    addInputVarsToSolver(solver);
    invNode().registerOutputVars(*_invariantGraph, solver);
    invNode().registerNode(*_invariantGraph, solver);

    std::vector<propagation::VarId> inputs;
    EXPECT_EQ(invNode().staticInputVarNodeIds().size(), 2);
    for (const auto& inputVarNodeId : invNode().staticInputVarNodeIds()) {
      EXPECT_NE(varId(inputVarNodeId), propagation::NULL_ID);
      inputs.emplace_back(varId(inputVarNodeId));
      solver.updateBounds(varId(inputVarNodeId), 0, 10, true);
    }

    EXPECT_NE(invNode().violationVarId(*_invariantGraph), propagation::NULL_ID);
    const propagation::VarId violationId =
        invNode().violationVarId(*_invariantGraph);

    std::vector<Int> values(inputs.size());
    solver.close();

    for (values.at(0) = solver.lowerBound(inputs.at(0));
         values.at(0) <= solver.upperBound(inputs.at(0)); ++values.at(0)) {
      for (values.at(1) = solver.lowerBound(inputs.at(1));
           values.at(1) <= solver.upperBound(inputs.at(1)); ++values.at(1)) {
        solver.beginMove();
        for (size_t i = 0; i < inputs.size(); ++i) {
          solver.setValue(inputs.at(i), values.at(i));
        }
        solver.endMove();

        solver.beginProbe();
        solver.query(violationId);
        solver.endProbe();

        if constexpr (Type != ViolationInvariantType::CONSTANT_FALSE) {
          EXPECT_EQ(solver.currentValue(violationId) > 0, isViolating(values));
        } else {
          EXPECT_NE(solver.currentValue(violationId) > 0, isViolating(values));
        }
      }
    }
  }
};

class ArrayBoolOrNodeTest
    : public AbstractArrayBoolOrNodeTest<ViolationInvariantType::NORMAL> {};

TEST_F(ArrayBoolOrNodeTest, Construction) { construction(); }

TEST_F(ArrayBoolOrNodeTest, Application) { application(); }

TEST_F(ArrayBoolOrNodeTest, Propagation) { propagation(); }

class ArrayBoolOrReifNodeTest
    : public AbstractArrayBoolOrNodeTest<ViolationInvariantType::REIFIED> {};

TEST_F(ArrayBoolOrReifNodeTest, Construction) { construction(); }

TEST_F(ArrayBoolOrReifNodeTest, Application) { application(); }

TEST_F(ArrayBoolOrReifNodeTest, Propagation) { propagation(); }

class ArrayBoolOrFalseNodeTest
    : public AbstractArrayBoolOrNodeTest<ViolationInvariantType::CONSTANT_FALSE> {};

TEST_F(ArrayBoolOrFalseNodeTest, Construction) { construction(); }

TEST_F(ArrayBoolOrFalseNodeTest, Application) { application(); }

TEST_F(ArrayBoolOrFalseNodeTest, Propagation) { propagation(); }

class ArrayBoolOrTrueNodeTest
    : public AbstractArrayBoolOrNodeTest<ViolationInvariantType::CONSTANT_TRUE> {};

TEST_F(ArrayBoolOrTrueNodeTest, Construction) { construction(); }

TEST_F(ArrayBoolOrTrueNodeTest, Application) { application(); }

TEST_F(ArrayBoolOrTrueNodeTest, Propagation) { propagation(); }

}  // namespace atlantis::testing