#include "invariantgraph/violationInvariantNodes/countLtNode.hpp"

#include "../parseHelper.hpp"

namespace invariantgraph {

std::unique_ptr<CountLtNode> CountLtNode::fromModelConstraint(
    const fznparser::Constraint& constraint, InvariantGraph& invariantGraph) {
  assert(hasCorrectSignature(acceptedNameNumArgPairs(), constraint));

  const fznparser::IntVarArray& xArg =
      std::get<fznparser::IntVarArray>(constraint.arguments().at(0));

  const fznparser::IntArg& yArg =
      std::get<fznparser::IntArg>(constraint.arguments().at(1));

  VarNodeId yVarNode =
      yArg.isParameter() ? nullptr : invariantGraph.createVarNode(yArg.var());

  const Int yParameter = yArg.isParameter() ? std::get<Int>(yArg) : -1;

  const fznparser::IntArg& cArg =
      std::get<fznparser::IntArg>(constraint.arguments().at(2));

  VarNodeId cVarNode =
      cArg.isParameter() ? nullptr : invariantGraph.createVarNode(cArg.var());

  const Int cParameter = cArg.isParameter() ? std::get<Int>(cArg) : -1;

  bool shouldHold = true;
  VarNodeId r = nullptr;

  if (constraint.arguments().size() == 4) {
    const fznparser::BoolArg& reified =
        std::get<fznparser::BoolArg>(constraint.arguments().back());

    if (reified.isFixed()) {
      shouldHold = reified.toParameter();
    } else {
      r = invariantGraph.createVarNode(reified.var());
    }
  }

  std::vector<VarNodeId> x = invariantGraph.createVarNodes(xArg);

  if (yArg.isParameter()) {
    if (cArg.isParameter()) {
      if (r != nullptr) {
        return std::make_unique<CountLtNode>(std::move(x), yParameter,
                                             cParameter, r);
      }
      return std::make_unique<CountLtNode>(std::move(x), yParameter, cParameter,
                                           shouldHold);
    }
    if (r != nullptr) {
      return std::make_unique<CountLtNode>(std::move(x), yParameter, cVarNode,
                                           r);
    }
    return std::make_unique<CountLtNode>(std::move(x), yParameter, cVarNode,
                                         shouldHold);
  }
  if (cArg.isParameter()) {
    if (r != nullptr) {
      return std::make_unique<CountLtNode>(std::move(x), yVarNode, cParameter,
                                           r);
    }
    return std::make_unique<CountLtNode>(std::move(x), yVarNode, cParameter,
                                         shouldHold);
  }
  if (r != nullptr) {
    return std::make_unique<CountLtNode>(std::move(x), yVarNode, cVarNode, r);
  }
  return std::make_unique<CountLtNode>(std::move(x), yVarNode, cVarNode,
                                       shouldHold);
}

void CountLtNode::registerOutputVariables(InvariantGraph& invariantGraph,
                                          Engine& engine) {
  if (violationVarId() == NULL_ID) {
    _intermediate = engine.makeIntVar(0, 0, 0);
    if (!_cIsParameter) {
      registerViolation(engine);
    } else {
      if (shouldHold()) {
        setViolationVarId(engine.makeIntView<LessEqualConst>(
            engine, _intermediate, _cParameter - 1));
      } else {
        assert(!isReified());
        setViolationVarId(engine.makeIntView<GreaterEqualConst>(
            engine, _intermediate, _cParameter));
      }
    }
  }
}

void CountLtNode::registerNode(InvariantGraph& invariantGraph, Engine& engine) {
  std::vector<VarId> engineInputs;
  assert(staticInputVarNodeIds().size() >=
         static_cast<size_t>(!_yIsParameter) +
             static_cast<size_t>(!_cIsParameter));
  const size_t vectorSize = staticInputVarNodeIds().size() -
                            static_cast<size_t>(!_yIsParameter) -
                            static_cast<size_t>(!_cIsParameter);

  engineInputs.resize(vectorSize);

  for (size_t i = 0; i < vectorSize; ++i) {
    engineInputs.at(i) = staticInputVarNodeIds().at(i)->varId();
  }

  assert(violationVarId() != NULL_ID);
  assert(_intermediate != NULL_ID);

  if (!_yIsParameter) {
    assert(yVarNode() != nullptr);
    assert(yVarNode()->varId() != NULL_ID);
    engine.makeInvariant<Count>(engine, _intermediate, yVarNode()->varId(),
                                engineInputs);
  } else {
    assert(yVarNode() == nullptr);
    engine.makeInvariant<CountConst>(engine, _intermediate, _yParameter,
                                     engineInputs);
  }
  if (!_cIsParameter) {
    assert(cVarNode() != nullptr);
    assert(cVarNode()->varId() != NULL_ID);
    if (shouldHold()) {
      engine.makeInvariant<LessThan>(engine, violationVarId(),
                                     cVarNode()->varId(), _intermediate);
    } else {
      // c >= count(x, y) -> count(x, y) <= c
      engine.makeInvariant<LessEqual>(engine, violationVarId(), _intermediate,
                                      cVarNode()->varId());
    }
  }
}

}  // namespace invariantgraph