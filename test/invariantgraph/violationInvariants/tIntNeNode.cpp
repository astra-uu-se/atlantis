#include "../nodeTestBase.hpp"
#include "invariantgraph/violationInvariantNodes/intNeNode.hpp"
#include "propagation/solver.hpp"

namespace atlantis::testing {

using namespace atlantis::invariantgraph;

static bool isViolating(std::vector<Int>& values) {
  for (size_t i = 0; i < values.size(); ++i) {
    for (size_t j = i + 1; j < values.size(); ++j) {
      if (values.at(i) == values.at(j)) {
        return true;
      }
    }
  }
  return false;
}

template <ViolationInvariantType Type>
class AbstractIntNeNodeTest : public NodeTestBase<IntNeNode> {
 public:
  VarNodeId a;
  VarNodeId b;
  VarNodeId r;

  void SetUp() override {
    NodeTestBase::SetUp();
    a = createIntVar(5, 10, "a");
    b = createIntVar(2, 7, "b");
    r = createBoolVar("r");

    if constexpr (Type == ViolationInvariantType::REIFIED) {
      _model->addConstraint(std::move(fznparser::Constraint(
          "int_ne_reif",
          std::vector<fznparser::Arg>{fznparser::IntArg{intVar(a)},
                                      fznparser::IntArg{intVar(b)},
                                      fznparser::BoolArg{boolVar(r)}})));

    } else {
      if constexpr (Type == ViolationInvariantType::NORMAL) {
        _model->addConstraint(std::move(fznparser::Constraint(
            "int_ne",
            std::vector<fznparser::Arg>{fznparser::IntArg{intVar(a)},
                                        fznparser::IntArg{intVar(b)}})));
      } else if constexpr (Type == ViolationInvariantType::CONSTANT_FALSE) {
        _model->addConstraint(std::move(fznparser::Constraint(
            "int_ne_reif",
            std::vector<fznparser::Arg>{fznparser::IntArg{intVar(a)},
                                        fznparser::IntArg{intVar(b)},
                                        fznparser::BoolArg{false}})));
      } else {
        _model->addConstraint(std::move(fznparser::Constraint(
            "int_ne_reif",
            std::vector<fznparser::Arg>{fznparser::IntArg{intVar(a)},
                                        fznparser::IntArg{intVar(b)},
                                        fznparser::BoolArg{true}})));
      }
    }

    makeInvNode(_model->constraints().front());
  }

  void construction() {
    expectInputTo(invNode());
    expectOutputOf(invNode());

    EXPECT_EQ(invNode().a(), a);
    EXPECT_EQ(invNode().b(), b);

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
    EXPECT_EQ(invNode().violationVarId(*_invariantGraph), propagation::NULL_ID);
    invNode().registerOutputVars(*_invariantGraph, solver);
    for (const auto& outputVarNodeId : invNode().outputVarNodeIds()) {
      EXPECT_NE(varId(outputVarNodeId), propagation::NULL_ID);
    }
    EXPECT_NE(invNode().violationVarId(*_invariantGraph), propagation::NULL_ID);
    invNode().registerNode(*_invariantGraph, solver);
    solver.close();

    // a and b
    EXPECT_EQ(solver.searchVars().size(), 2);

    // a, b and the violation
    EXPECT_EQ(solver.numVars(), 3);

    // notEqual
    EXPECT_EQ(solver.numInvariants(), 1);

    EXPECT_EQ(solver.lowerBound(invNode().violationVarId(*_invariantGraph)), 0);
    EXPECT_GT(solver.upperBound(invNode().violationVarId(*_invariantGraph)), 0);
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
    }

    EXPECT_NE(invNode().violationVarId(*_invariantGraph), propagation::NULL_ID);
    const propagation::VarId violationId =
        invNode().violationVarId(*_invariantGraph);
    EXPECT_EQ(inputs.size(), 2);

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

class IntNeNodeTest
    : public AbstractIntNeNodeTest<ViolationInvariantType::NORMAL> {};

TEST_F(IntNeNodeTest, Construction) { construction(); }

TEST_F(IntNeNodeTest, Application) { application(); }

TEST_F(IntNeNodeTest, Propagation) { propagation(); }

class IntNeReifNodeTest
    : public AbstractIntNeNodeTest<ViolationInvariantType::REIFIED> {};

TEST_F(IntNeReifNodeTest, Construction) { construction(); }

TEST_F(IntNeReifNodeTest, Application) { application(); }

TEST_F(IntNeReifNodeTest, Propagation) { propagation(); }

class IntNeFalseNodeTest
    : public AbstractIntNeNodeTest<ViolationInvariantType::CONSTANT_FALSE> {};

TEST_F(IntNeFalseNodeTest, Construction) { construction(); }

TEST_F(IntNeFalseNodeTest, Application) { application(); }

TEST_F(IntNeFalseNodeTest, Propagation) { propagation(); }

class IntNeTrueNodeTest
    : public AbstractIntNeNodeTest<ViolationInvariantType::CONSTANT_TRUE> {};

TEST_F(IntNeTrueNodeTest, Construction) { construction(); }

TEST_F(IntNeTrueNodeTest, Application) { application(); }

TEST_F(IntNeTrueNodeTest, Propagation) { propagation(); }

}  // namespace atlantis::testing