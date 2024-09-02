#include "atlantis/invariantgraph/invariantNodes/intLinearNode.hpp"

#include <algorithm>
#include <utility>

#include "../parseHelper.hpp"
#include "atlantis/invariantgraph/implicitConstraintNodes/intLinEqImplicitNode.hpp"
#include "atlantis/invariantgraph/views/intScalarNode.hpp"
#include "atlantis/propagation/invariants/linear.hpp"
#include "atlantis/propagation/views/intOffsetView.hpp"
#include "atlantis/propagation/views/scalarView.hpp"

namespace atlantis::invariantgraph {

IntLinearNode::IntLinearNode(IInvariantGraph& graph, std::vector<Int>&& coeffs,
                             std::vector<VarNodeId>&& vars, VarNodeId output,
                             Int offset)
    : InvariantNode(graph, {output}, std::move(vars)),
      _coeffs(std::move(coeffs)),
      _offset(offset) {}

void IntLinearNode::init(InvariantNodeId id) {
  InvariantNode::init(id);
  assert(invariantGraphConst()
             .varNodeConst(outputVarNodeIds().front())
             .isIntVar());
  assert(
      std::all_of(staticInputVarNodeIds().begin(),
                  staticInputVarNodeIds().end(), [&](const VarNodeId vId) {
                    return invariantGraphConst().varNodeConst(vId).isIntVar();
                  }));
}

void IntLinearNode::updateState() {
  // Remove duplicates:
  for (Int i = 0; i < static_cast<Int>(staticInputVarNodeIds().size()); ++i) {
    for (Int j = static_cast<Int>(staticInputVarNodeIds().size()) - 1; j > i;
         --j) {
      if (staticInputVarNodeIds().at(i) == staticInputVarNodeIds().at(j)) {
        _coeffs.at(i) += _coeffs.at(j);
        _coeffs.erase(_coeffs.begin() + j);
        eraseStaticInputVarNode(j);
      }
    }
  }

  std::vector<Int> indicesToRemove;
  indicesToRemove.reserve(staticInputVarNodeIds().size());

  for (Int i = 0; i < static_cast<Int>(staticInputVarNodeIds().size()); ++i) {
    const auto& inputNode =
        invariantGraphConst().varNodeConst(staticInputVarNodeIds().at(i));
    if (inputNode.isFixed() || _coeffs.at(i) == 0) {
      _offset += _coeffs.at(i) * inputNode.lowerBound();
      indicesToRemove.emplace_back(i);
    }
  }

  for (Int i = indicesToRemove.size() - 1; i >= 0; --i) {
    removeStaticInputVarNode(staticInputVarNodeIds().at(indicesToRemove.at(i)));
    _coeffs.erase(_coeffs.begin() + indicesToRemove.at(i));
  }

  Int lb = _offset;
  Int ub = _offset;
  for (size_t i = 0; i < staticInputVarNodeIds().size(); ++i) {
    const Int v1 =
        _coeffs.at(i) *
        invariantGraph().varNode(staticInputVarNodeIds().at(i)).lowerBound();
    const Int v2 =
        _coeffs.at(i) *
        invariantGraph().varNode(staticInputVarNodeIds().at(i)).upperBound();
    lb += std::min(v1, v2);
    ub += std::max(v1, v2);
  }

  auto& outputNode = invariantGraph().varNode(outputVarNodeIds().front());

  /*
  outputNode.removeValuesBelow(lb);
  outputNode.removeValuesAbove(ub);
  */

  if (staticInputVarNodeIds().empty()) {
    outputNode.fixToValue(_offset);
    setState(InvariantNodeState::SUBSUMED);
  }
}

bool IntLinearNode::canBeMadeImplicit() const {
  return std::all_of(_coeffs.begin(), _coeffs.end(),
                     [](const Int& coeff) { return std::abs(coeff) == 1; }) &&
         std::all_of(staticInputVarNodeIds().begin(),
                     staticInputVarNodeIds().end(),
                     [&](const VarNodeId vId) {
                       return invariantGraphConst()
                           .varNodeConst(vId)
                           .definingNodes()
                           .empty();
                     }) &&
         invariantGraphConst()
             .varNodeConst(outputVarNodeIds().front())
             .definingNodes()
             .empty();
}

bool IntLinearNode::makeImplicit() {
  if (!canBeMadeImplicit()) {
    return false;
  }

  _coeffs.emplace_back(-1);

  std::vector<VarNodeId> inputVarNodeIds(staticInputVarNodeIds());
  inputVarNodeIds.emplace_back(outputVarNodeIds().front());

  invariantGraph().addImplicitConstraintNode(
      std::make_shared<IntLinEqImplicitNode>(
          invariantGraph(), std::move(_coeffs), std::move(inputVarNodeIds),
          _offset));

  return true;
}

void IntLinearNode::registerOutputVars() {
  if (staticInputVarNodeIds().size() == 1) {
    invariantGraph()
        .varNode(outputVarNodeIds().front())
        .setVarId(solver().makeIntView<propagation::ScalarView>(
            solver(), invariantGraph().varId(staticInputVarNodeIds().front()),
            _coeffs.front(), _offset));
    return;
  } else if (!staticInputVarNodeIds().empty()) {
    if (_offset == 0) {
      makeSolverVar(outputVarNodeIds().front());
      assert(invariantGraph().varId(outputVarNodeIds().front()).isVar());
    } else if (_intermediate == propagation::NULL_ID) {
      _intermediate = solver().makeIntVar(0, 0, 0);
      invariantGraph()
          .varNode(outputVarNodeIds().front())
          .setVarId(solver().makeIntView<propagation::IntOffsetView>(
              solver(), _intermediate, _offset));
    }
  }
  assert(std::all_of(outputVarNodeIds().begin(), outputVarNodeIds().end(),
                     [&](const VarNodeId vId) {
                       return invariantGraphConst().varNodeConst(vId).varId() !=
                              propagation::NULL_ID;
                     }));
}

void IntLinearNode::registerNode() {
  if (staticInputVarNodeIds().size() <= 1) {
    return;
  }
  assert(invariantGraph().varId(outputVarNodeIds().front()) !=
         propagation::NULL_ID);
  assert(_intermediate == propagation::NULL_ID
             ? invariantGraph().varId(outputVarNodeIds().front()).isVar()
             : invariantGraph().varId(outputVarNodeIds().front()).isView());
  assert(_intermediate == propagation::NULL_ID || _intermediate.isVar());

  std::vector<propagation::VarViewId> solverVars;
  std::transform(
      staticInputVarNodeIds().begin(), staticInputVarNodeIds().end(),
      std::back_inserter(solverVars), [&](const VarNodeId varNodeId) {
        assert(invariantGraph().varId(varNodeId) != propagation::NULL_ID);
        return invariantGraph().varId(varNodeId);
      });
  solver().makeInvariant<propagation::Linear>(
      solver(),
      _intermediate == propagation::NULL_ID
          ? invariantGraph().varId(outputVarNodeIds().front())
          : _intermediate,
      std::vector<Int>(_coeffs), std::move(solverVars));
}

const std::vector<Int>& IntLinearNode::coeffs() const { return _coeffs; }

std::string IntLinearNode::dotLangIdentifier() const { return "int_linear"; }

}  // namespace atlantis::invariantgraph
