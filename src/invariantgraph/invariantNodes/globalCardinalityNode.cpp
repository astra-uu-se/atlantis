#include "atlantis/invariantgraph/invariantNodes/globalCardinalityNode.hpp"

#include <algorithm>
#include <numeric>
#include <stack>
#include <utility>
#include <vector>

#include "../parseHelper.hpp"
#include "atlantis/invariantgraph/constraintSolver.hpp"
#include "atlantis/invariantgraph/invariantGraph.hpp"
#include "atlantis/invariantgraph/invariantNodes/countNode.hpp"
#include "atlantis/invariantgraph/varNode.hpp"
#include "atlantis/propagation/invariants/globalCardinalityOpen.hpp"
#include "atlantis/propagation/solverBase.hpp"
#include "atlantis/propagation/views/intOffsetView.hpp"

namespace atlantis::invariantgraph {

GlobalCardinalityNode::GlobalCardinalityNode(InvariantGraph& graph,
                                             std::vector<VarNodeId>&& inputs,
                                             std::vector<Int>&& cover,
                                             std::vector<VarNodeId>&& counts,
                                             std::vector<Int>&& countOffsets)
    : InvariantNode(graph, std::move(counts), std::move(inputs)),
      _cover(std::move(cover)),
      _countOffsets(std::move(countOffsets)) {
  _countOffsets.resize(_cover.size(), 0);
  assert(_cover.size() == outputVarNodeIds().size());
  if (_cover.empty()) {
    setState(InvariantNodeState::SUBSUMED);
  }
}

void GlobalCardinalityNode::postConstraint() {
  InvariantNode::postConstraint();
  constraintSolver().fzn_global_cardinality(
      toConstraintVarIds(invariantGraphConst(), staticInputVarNodeIds()),
      _cover, toConstraintVarIds(invariantGraphConst(), outputVarNodeIds()),
      true);
}

void GlobalCardinalityNode::init(const InvariantNodeId id) {
  InvariantNode::init(id);
  assert(std::ranges::all_of(
      outputVarNodeIds().begin(), outputVarNodeIds().end(),
      [&](const VarNodeId vId) {
        return invariantGraphConst().varNodeConst(vId).isIntVar();
      }));
  assert(std::ranges::all_of(
      staticInputVarNodeIds().begin(), staticInputVarNodeIds().end(),
      [&](const VarNodeId vId) {
        return invariantGraphConst().varNodeConst(vId).isIntVar();
      }));
}

void GlobalCardinalityNode::updateState() {
  // GCC can define the same output multiple times. Therefore, split all outputs
  // that are defined multiple times:
  postAllEqualOnReplacedVars(invariantGraph(), splitOutputVarNodes());

  InvariantNode::updateState();

  const auto [varsToRemove, coverIndicesToRemove] =
      gccUpdateState(invariantGraphConst(), staticInputVarNodeIds(), _cover);

  for (const VarNodeId vId : varsToRemove) {
    removeStaticInputVarNode(vId);
  }

  for (Int i = static_cast<Int>(coverIndicesToRemove->size()) - 1; i >= 0;
       --i) {
    _cover.erase(_cover.begin() + i);
    removeOutputAtIndex(i);
  }

  if (_cover.empty() || staticInputVarNodeIds().empty()) {
    setState(InvariantNodeState::SUBSUMED);
  }
}

bool GlobalCardinalityNode::canBeReplaced() const {
  return state() == InvariantNodeState::ACTIVE && _cover.size() <= 1;
}

bool GlobalCardinalityNode::replace() {
  if (!canBeReplaced()) {
    return false;
  }
  assert(_cover.size() == 1);
  invariantGraph().addInvariantNode(std::make_shared<CountNode>(
      invariantGraph(), outputVarNodeIds().front(),
      std::vector<VarNodeId>(staticInputVarNodeIds()), _cover.front(),
      _countOffsets.front()));
  return true;
}

void GlobalCardinalityNode::registerOutputVars(propagation::SolverBase& solver,
                                               SolverMapping& mapping) const {
  for (size_t i = 0; i < _cover.size(); ++i) {
    assert(std::ranges::none_of(
        outputVarNodeIds().begin(),
        outputVarNodeIds().begin() + static_cast<Int>(i),
        [&](const VarNodeId vId) { return vId == outputVarNodeIds().at(i); }));

    if (_countOffsets[i] == 0) {
      assert(mapping.solverId(outputVarNodeIds().at(i)) ==
             propagation::NULL_ID);
      makeSolverVar(outputVarNodeIds().at(i), solver, mapping);
    } else {
      assert(mapping.solverId(outputVarNodeIds().at(i)) ==
             propagation::NULL_ID);
      mapping.setIntermediateId(id(), i, solver.makeIntVar(0, 0, 0));
      mapping.setSolverId(
          outputVarNodeIds().at(i),
          solver.makeIntView<propagation::IntOffsetView>(
              solver, mapping.intermediateId(id(), i), _countOffsets[i]));
    }
  }
  assert(std::ranges::all_of(
      outputVarNodeIds().begin(), outputVarNodeIds().end(),
      [&](const VarNodeId vId) {
        return mapping.solverId(vId) != propagation::NULL_ID;
      }));
}

void GlobalCardinalityNode::registerNode(propagation::SolverBase& solver,
                                         SolverMapping& mapping) const {
  std::vector<propagation::VarViewId> inputVarIds;
  std::ranges::transform(staticInputVarNodeIds(),
                         std::back_inserter(inputVarIds),
                         [&](const auto& id) { return mapping.solverId(id); });

  std::vector<propagation::VarViewId> outputVarIds;
  outputVarIds.reserve(outputVarNodeIds().size());
  for (size_t i = 0; i < _cover.size(); ++i) {
    assert(mapping.intermediateId(id(), i) == propagation::NULL_ID ||
           mapping.intermediateId(id(), i).isVar());

    outputVarIds.emplace_back(mapping.intermediateId(id(), i) ==
                                      propagation::NULL_ID
                                  ? mapping.solverId(outputVarNodeIds().at(i))
                                  : mapping.intermediateId(id(), i));
  }

  solver.makeInvariant<propagation::GlobalCardinalityOpen>(
      solver, std::move(outputVarIds), std::move(inputVarIds),
      std::vector<Int>(_cover));
}

std::string GlobalCardinalityNode::dotLangIdentifier() const {
  std::string s{"global_cardinality ["};
  for (size_t i = 0; i < _cover.size(); ++i) {
    s += std::to_string(_cover.at(i));
    if (i < _cover.size() - 1) {
      s += ", ";
    }
  }
  s += ']';
  return s;
}

}  // namespace atlantis::invariantgraph
