#include "invariantgraph/violationInvariantNodes/countLeqNode.hpp"

#include "../parseHelper.hpp"

namespace invariantgraph {

CountLeqNode::CountLeqNode(std::vector<VarNodeId>&& x, VarNodeId y,
                           Int yParameter, VarNodeId c, Int cParameter,
                           VarNodeId r)
    : ViolationInvariantNode(append(std::move(x), y, c), r),
      _yIsParameter(y == NULL_NODE_ID),
      _yParameter(yParameter),
      _cIsParameter(c == NULL_NODE_ID),
      _cParameter(cParameter) {}

CountLeqNode::CountLeqNode(std::vector<VarNodeId>&& x, VarNodeId y,
                           Int yParameter, VarNodeId c, Int cParameter,
                           bool shouldHold)
    : ViolationInvariantNode(append(std::move(x), y, c), shouldHold),
      _yIsParameter(y == NULL_NODE_ID),
      _yParameter(yParameter),
      _cIsParameter(c == NULL_NODE_ID),
      _cParameter(cParameter) {}

CountLeqNode::CountLeqNode(std::vector<VarNodeId>&& x, VarNodeId y, VarNodeId c,
                           VarNodeId r)
    : CountLeqNode(std::move(x), y, 0, c, 0, r) {}

CountLeqNode::CountLeqNode(std::vector<VarNodeId>&& x, Int yParameter,
                           VarNodeId c, VarNodeId r)
    : CountLeqNode(std::move(x), NULL_NODE_ID, yParameter, c, 0, r) {}

CountLeqNode::CountLeqNode(std::vector<VarNodeId>&& x, VarNodeId y,
                           Int cParameter, VarNodeId r)
    : CountLeqNode(std::move(x), y, 0, NULL_NODE_ID, cParameter, r) {}

CountLeqNode::CountLeqNode(std::vector<VarNodeId>&& x, Int yParameter,
                           Int cParameter, VarNodeId r)
    : CountLeqNode(std::move(x), NULL_NODE_ID, yParameter, NULL_NODE_ID,
                   cParameter, r) {}

CountLeqNode::CountLeqNode(std::vector<VarNodeId>&& x, VarNodeId y, VarNodeId c,
                           bool shouldHold)
    : CountLeqNode(std::move(x), y, 0, c, 0, shouldHold) {}

CountLeqNode::CountLeqNode(std::vector<VarNodeId>&& x, Int yParameter,
                           VarNodeId c, bool shouldHold)
    : CountLeqNode(std::move(x), NULL_NODE_ID, yParameter, c, 0, shouldHold) {}

CountLeqNode::CountLeqNode(std::vector<VarNodeId>&& x, VarNodeId y,
                           Int cParameter, bool shouldHold)
    : CountLeqNode(std::move(x), y, 0, NULL_NODE_ID, cParameter, shouldHold) {}

CountLeqNode::CountLeqNode(std::vector<VarNodeId>&& x, Int yParameter,
                           Int cParameter, bool shouldHold)
    : CountLeqNode(std::move(x), NULL_NODE_ID, yParameter, NULL_NODE_ID,
                   cParameter, shouldHold) {}

std::unique_ptr<CountLeqNode> CountLeqNode::fromModelConstraint(
    const fznparser::Constraint& constraint, InvariantGraph& invariantGraph) {
  assert(hasCorrectSignature(acceptedNameNumArgPairs(), constraint));

  const fznparser::IntVarArray& xArg =
      std::get<fznparser::IntVarArray>(constraint.arguments().at(0));

  const fznparser::IntArg& yArg =
      std::get<fznparser::IntArg>(constraint.arguments().at(1));

  VarNodeId yVarNode = yArg.isParameter()
                           ? NULL_NODE_ID
                           : invariantGraph.createVarNode(yArg.var());

  const Int yParameter = yArg.isParameter() ? std::get<Int>(yArg) : -1;

  const fznparser::IntArg& cArg =
      std::get<fznparser::IntArg>(constraint.arguments().at(2));

  VarNodeId cVarNode = cArg.isParameter()
                           ? NULL_NODE_ID
                           : invariantGraph.createVarNode(cArg.var());

  const Int cParameter = cArg.isParameter() ? std::get<Int>(cArg) : -1;

  bool shouldHold = true;
  VarNodeId r = NULL_NODE_ID;

  if (constraint.arguments().size() == 4) {
    const fznparser::BoolArg& reified =
        std::get<fznparser::BoolArg>(constraint.arguments().back());

    if (reified.isFixed()) {
      shouldHold = reified.toParameter();
    } else {
      r = invariantGraph.createVarNode(reified.var());
    }
  }

  if (yArg.isParameter()) {
    if (cArg.isParameter()) {
      if (r != NULL_NODE_ID) {
        return std::make_unique<CountLeqNode>(
            invariantGraph.createVarNodes(xArg), yParameter, cParameter, r);
      }
      return std::make_unique<CountLeqNode>(invariantGraph.createVarNodes(xArg),
                                            yParameter, cParameter, shouldHold);
    }
    if (r != NULL_NODE_ID) {
      return std::make_unique<CountLeqNode>(invariantGraph.createVarNodes(xArg),
                                            yParameter, cVarNode, r);
    }
    return std::make_unique<CountLeqNode>(invariantGraph.createVarNodes(xArg),
                                          yParameter, cVarNode, shouldHold);
  }
  if (cArg.isParameter()) {
    if (r != NULL_NODE_ID) {
      return std::make_unique<CountLeqNode>(invariantGraph.createVarNodes(xArg),
                                            yVarNode, cParameter, r);
    }
    return std::make_unique<CountLeqNode>(invariantGraph.createVarNodes(xArg),
                                          yVarNode, cParameter, shouldHold);
  }
  if (r != NULL_NODE_ID) {
    return std::make_unique<CountLeqNode>(invariantGraph.createVarNodes(xArg),
                                          yVarNode, cVarNode, r);
  }
  return std::make_unique<CountLeqNode>(invariantGraph.createVarNodes(xArg),
                                        yVarNode, cVarNode, shouldHold);
}

void CountLeqNode::registerOutputVariables(InvariantGraph& invariantGraph,
                                           Engine& engine) {
  if (violationVarId(invariantGraph) == NULL_ID) {
    _intermediate = engine.makeIntVar(0, 0, 0);
    if (!_cIsParameter) {
      registerViolation(invariantGraph, engine);
    } else {
      if (shouldHold()) {
        // Constrains c to be less than or equal to the number of occurrences of
        // y in x.
        // occurrences >= c
        setViolationVarId(invariantGraph,
                          engine.makeIntView<GreaterEqualConst>(
                              engine, _intermediate, _cParameter));
      } else {
        assert(!isReified());
        // occurrences < c
        // occurrences <= c - 1
        setViolationVarId(invariantGraph,
                          engine.makeIntView<LessEqualConst>(
                              engine, _intermediate, _cParameter - 1));
      }
    }
  }
}

void CountLeqNode::registerNode(InvariantGraph& invariantGraph,
                                Engine& engine) {
  std::vector<VarId> engineInputs;
  assert(staticInputVarNodeIds().size() >=
         static_cast<size_t>(!_yIsParameter) +
             static_cast<size_t>(!_cIsParameter));
  const size_t vectorSize = staticInputVarNodeIds().size() -
                            static_cast<size_t>(!_yIsParameter) -
                            static_cast<size_t>(!_cIsParameter);

  engineInputs.resize(vectorSize);

  for (size_t i = 0; i < vectorSize; ++i) {
    engineInputs.at(i) = invariantGraph.varId(staticInputVarNodeIds().at(i));
  }

  assert(violationVarId(invariantGraph) != NULL_ID);
  assert(_intermediate != NULL_ID);

  if (!_yIsParameter) {
    assert(yVarNode() != NULL_NODE_ID);
    assert(invariantGraph.varId(yVarNode()) != NULL_ID);
    engine.makeInvariant<Count>(engine, _intermediate,
                                invariantGraph.varId(yVarNode()), engineInputs);
  } else {
    assert(yVarNode() == NULL_NODE_ID);
    engine.makeInvariant<CountConst>(engine, _intermediate, _yParameter,
                                     engineInputs);
  }
  if (!_cIsParameter) {
    assert(cVarNode() != NULL_NODE_ID);
    assert(invariantGraph.varId(cVarNode()) != NULL_ID);
    if (shouldHold()) {
      engine.makeInvariant<LessEqual>(engine, violationVarId(invariantGraph),
                                      invariantGraph.varId(cVarNode()),
                                      _intermediate);
    } else {
      // c > count(x, y) -> count(x, y) < c
      engine.makeInvariant<LessThan>(engine, violationVarId(invariantGraph),
                                     _intermediate,
                                     invariantGraph.varId(cVarNode()));
    }
  }
}

[[nodiscard]] VarNodeId CountLeqNode::yVarNode() const {
  return _yIsParameter ? VarNodeId(NULL_NODE_ID)
                       : staticInputVarNodeIds().at(
                             staticInputVarNodeIds().size() -
                             (1 + static_cast<size_t>(!_cIsParameter)));
}

[[nodiscard]] VarNodeId CountLeqNode::cVarNode() const {
  return _cIsParameter ? VarNodeId(NULL_NODE_ID)
                       : staticInputVarNodeIds().back();
}

}  // namespace invariantgraph