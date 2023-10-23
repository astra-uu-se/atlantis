#include <gmock/gmock.h>

#include "../nodeTestBase.hpp"
#include "invariantgraph/violationInvariantNodes/allEqualNode.hpp"
#include "propagation/propagationEngine.hpp"

namespace atlantis::testing {

using namespace atlantis::invariantgraph;

using ::testing::ContainerEq;

static bool isViolating(const std::vector<Int>& values) {
  for (size_t i = 0; i < values.size(); i++) {
    for (size_t j = i + 1; j < values.size(); j++) {
      if (values.at(i) != values.at(j)) {
        return true;
      }
    }
  }
  return false;
}

template <ConstraintType Type>
class AbstractAllEqualNodeTest : public NodeTestBase<AllEqualNode> {
 public:
  VarNodeId a;
  VarNodeId b;
  VarNodeId c;
  VarNodeId d;
  VarNodeId r;

  void SetUp() override {
    NodeTestBase::SetUp();
    a = createIntVar(5, 10, "a");
    b = createIntVar(2, 7, "b");
    c = createIntVar(2, 7, "c");
    d = createIntVar(2, 7, "d");
    r = createBoolVar("r");

    fznparser::IntVarArray inputs{""};
    inputs.append(intVar(a));
    inputs.append(intVar(b));
    inputs.append(intVar(c));
    inputs.append(intVar(d));
    if constexpr (Type == ConstraintType::REIFIED) {
      _model->addConstraint(std::move(
          fznparser::Constraint("fzn_all_equal_int_reif",
                                std::vector<fznparser::Arg>{
                                    inputs, fznparser::BoolArg{boolVar(r)}})));
    } else {
      if constexpr (Type == ConstraintType::NORMAL) {
        _model->addConstraint(std::move(fznparser::Constraint(
            "fzn_all_equal_int", std::vector<fznparser::Arg>{inputs})));
      } else if constexpr (Type == ConstraintType::CONSTANT_FALSE) {
        _model->addConstraint(std::move(fznparser::Constraint(
            "fzn_all_equal_int_reif",
            std::vector<fznparser::Arg>{inputs, fznparser::BoolArg{false}})));
      } else {
        _model->addConstraint(std::move(fznparser::Constraint(
            "fzn_all_equal_int_reif",
            std::vector<fznparser::Arg>{inputs, fznparser::BoolArg{true}})));
      }
    }

    makeInvNode(_model->constraints().front());
  }

  void construction() {
    expectInputTo(invNode());
    expectOutputOf(invNode());

    EXPECT_EQ(invNode().staticInputVarNodeIds().size(), 4);
    std::vector<VarNodeId> expectedVars{a, b, c, d};
    EXPECT_EQ(invNode().staticInputVarNodeIds(), expectedVars);
    EXPECT_THAT(expectedVars, ContainerEq(invNode().staticInputVarNodeIds()));
    if constexpr (Type != ConstraintType::REIFIED) {
      EXPECT_FALSE(invNode().isReified());
      EXPECT_EQ(invNode().reifiedViolationNodeId(), NULL_NODE_ID);
    } else {
      EXPECT_TRUE(invNode().isReified());
      EXPECT_NE(invNode().reifiedViolationNodeId(), NULL_NODE_ID);
      EXPECT_EQ(invNode().reifiedViolationNodeId(), r);
    }
  }

  void application() {
    propagation::PropagationEngine engine;
    engine.open();
    addInputVarsToEngine(engine);
    for (const auto& outputVarNodeId : invNode().outputVarNodeIds()) {
      EXPECT_EQ(varId(outputVarNodeId), propagation::NULL_ID);
    }
    EXPECT_EQ(invNode().violationVarId(*_invariantGraph), propagation::NULL_ID);
    invNode().registerOutputVariables(*_invariantGraph, engine);
    for (const auto& outputVarNodeId : invNode().outputVarNodeIds()) {
      EXPECT_NE(varId(outputVarNodeId), propagation::NULL_ID);
    }
    EXPECT_NE(invNode().violationVarId(*_invariantGraph), propagation::NULL_ID);
    invNode().registerNode(*_invariantGraph, engine);
    engine.close();

    // a, b, c and d
    EXPECT_EQ(engine.searchVariables().size(), 4);

    // a, b, c, d and the violation
    EXPECT_EQ(engine.numVariables(), 5);

    // alldifferent
    EXPECT_EQ(engine.numInvariants(), 1);

    EXPECT_EQ(engine.lowerBound(invNode().violationVarId(*_invariantGraph)), 0);
    EXPECT_GT(engine.upperBound(invNode().violationVarId(*_invariantGraph)), 0);
  }

  void propagation() {
    propagation::PropagationEngine engine;
    engine.open();
    addInputVarsToEngine(engine);
    invNode().registerOutputVariables(*_invariantGraph, engine);
    invNode().registerNode(*_invariantGraph, engine);

    std::vector<propagation::VarId> inputs;
    EXPECT_EQ(invNode().staticInputVarNodeIds().size(), 4);
    for (const auto& inputVarNodeId : invNode().staticInputVarNodeIds()) {
      EXPECT_NE(varId(inputVarNodeId), propagation::NULL_ID);
      inputs.emplace_back(varId(inputVarNodeId));
    }

    EXPECT_NE(invNode().violationVarId(*_invariantGraph), propagation::NULL_ID);
    const propagation::VarId violationId =
        invNode().violationVarId(*_invariantGraph);
    EXPECT_EQ(inputs.size(), 4);

    std::vector<Int> values(inputs.size());
    engine.close();

    for (values.at(0) = engine.lowerBound(inputs.at(0));
         values.at(0) <= engine.upperBound(inputs.at(0)); ++values.at(0)) {
      for (values.at(1) = engine.lowerBound(inputs.at(1));
           values.at(1) <= engine.upperBound(inputs.at(1)); ++values.at(1)) {
        for (values.at(2) = engine.lowerBound(inputs.at(2));
             values.at(2) <= engine.upperBound(inputs.at(2)); ++values.at(2)) {
          for (values.at(3) = engine.lowerBound(inputs.at(3));
               values.at(3) <= engine.upperBound(inputs.at(3));
               ++values.at(3)) {
            engine.beginMove();
            for (size_t i = 0; i < inputs.size(); ++i) {
              engine.setValue(inputs.at(i), values.at(i));
            }
            engine.endMove();

            engine.beginProbe();
            engine.query(violationId);
            engine.endProbe();

            if constexpr (Type != ConstraintType::CONSTANT_FALSE) {
              EXPECT_EQ(engine.currentValue(violationId) > 0,
                        isViolating(values));
            } else {
              EXPECT_NE(engine.currentValue(violationId) > 0,
                        isViolating(values));
            }
          }
        }
      }
    }
  }
};

class AllEqualNodeTest
    : public AbstractAllEqualNodeTest<ConstraintType::NORMAL> {};

TEST_F(AllEqualNodeTest, Construction) { construction(); }

TEST_F(AllEqualNodeTest, Application) { application(); }

TEST_F(AllEqualNodeTest, Propagation) { propagation(); }

class AllEqualReifNodeTest
    : public AbstractAllEqualNodeTest<ConstraintType::REIFIED> {};

TEST_F(AllEqualReifNodeTest, Construction) { construction(); }

TEST_F(AllEqualReifNodeTest, Application) { application(); }

TEST_F(AllEqualReifNodeTest, Propagation) { propagation(); }

class AllEqualFalseNodeTest
    : public AbstractAllEqualNodeTest<ConstraintType::CONSTANT_FALSE> {};

TEST_F(AllEqualFalseNodeTest, Construction) { construction(); }

TEST_F(AllEqualFalseNodeTest, Application) { application(); }

TEST_F(AllEqualFalseNodeTest, Propagation) { propagation(); }

class AllEqualTrueNodeTest
    : public AbstractAllEqualNodeTest<ConstraintType::CONSTANT_TRUE> {};

TEST_F(AllEqualTrueNodeTest, Construction) { construction(); }

TEST_F(AllEqualTrueNodeTest, Application) { application(); }

TEST_F(AllEqualTrueNodeTest, Propagation) { propagation(); }
}  // namespace atlantis::testing