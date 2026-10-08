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

GlobalCardinalityNode::GlobalCardinalityNode(
    InvariantGraph& graph, std::vector<std::shared_ptr<VarNode>>&& inputs,
    std::vector<Int>&& cover, std::vector<std::shared_ptr<VarNode>>&& counts,
    std::vector<Int>&& countOffsets)
    : InvariantNode(graph, std::move(counts), std::move(inputs)),
      _cover(std::move(cover)),
      _countOffsets(std::move(countOffsets)) {
  _countOffsets.resize(_cover.size(), 0);
  assert(_cover.size() == outputVarNodes().size());
  if (_cover.empty()) {
    setState(InvariantNodeState::SUBSUMED);
  }
}

void GlobalCardinalityNode::postConstraint() {
  InvariantNode::postConstraint();
  constraintSolver().fzn_global_cardinality(
      toConstraintVarIds(invariantGraphConst(), staticInputVarNodes()), _cover,
      toConstraintVarIds(invariantGraphConst(), outputVarNodes()), true);
}

void GlobalCardinalityNode::removeOutputVarNode(VarNode& outputVarNodeId) {
  for (Int i = static_cast<Int>(_cover.size()) - 1; i >= 0; --i) {
    if (&outputVarNode(i) == &outputVarNodeId) {
      _cover.erase(_cover.begin() + i);
      _countOffsets.erase(_countOffsets.begin() + i);
    }
  }
  InvariantNode::removeOutputVarNode(outputVarNodeId);
  assert(_cover.size() == outputVarNodes().size());
}

void GlobalCardinalityNode::removeOutputAtIndex(const size_t index) {
  assert(index < _cover.size());
  _cover.erase(_cover.begin() + static_cast<Int>(index));
  _countOffsets.erase(_countOffsets.begin() + static_cast<Int>(index));
  InvariantNode::removeOutputAtIndex(index);
  assert(_cover.size() == outputVarNodes().size());
}

void GlobalCardinalityNode::init() {
  InvariantNode::init();

  assert(std::ranges::all_of(
      outputVarNodes().begin(), outputVarNodes().end(),
      [&](const std::shared_ptr<VarNode>& vNode) { return vNode->isIntVar(); }));
  assert(std::ranges::all_of(
      staticInputVarNodes().begin(), staticInputVarNodes().end(),
      [&](const std::shared_ptr<VarNode>& vNode) { return vNode->isIntVar(); }));
}

void GlobalCardinalityNode::updateState() {
  // Remove duplicated covers:
  for (Int index = 0; index < static_cast<Int>(_cover.size()); ++index) {
    for (Int dupIndex = static_cast<Int>(_cover.size()) - 1; dupIndex > index;
         --dupIndex) {
      if (_cover[index] == _cover[dupIndex]) {
        _countOffsets[index] += _countOffsets[dupIndex];

        const std::shared_ptr<VarNode>& duplicateNode =
            outputVarNodes().at(dupIndex);
        removeOutputAtIndex(dupIndex);
        invariantGraph().replaceVarNode(*duplicateNode,
                                        outputVarNode(index));
      }
    }
  }
  // GCC can define the same output multiple times. Therefore, split all outputs
  // that are defined multiple times:
  postAllEqualOnReplacedVars(invariantGraph(), splitOutputVarNodes());

  InvariantNode::updateState();

  const auto [varsToRemove, coverIndicesToRemove] = gccUpdateState(
      invariantGraphConst(), staticInputVarNodes(), _cover, _countOffsets);

  for (Int i = static_cast<Int>(coverIndicesToRemove->size()) - 1; i >= 0;
       --i) {
    removeOutputAtIndex(i);
  }

  if (_cover.empty() || staticInputVarNodes().empty()) {
    setState(InvariantNodeState::SUBSUMED);
    return;
  }

  for (const auto& var : varsToRemove) {
    removeStaticInputVarNode(*var);
  }

  if (staticInputVarNodes().empty()) {
    for (const auto& outputVar : outputVarNodes()) {
      outputVar->tightenDomainType();
    }
    setState(InvariantNodeState::SUBSUMED);
  }
}

bool GlobalCardinalityNode::constrainsOutput(
    const VarNode& output) const {
  for (size_t i = 0; i < _cover.size(); ++i) {
    if (&outputVarNode(i) != &output) {
      continue;
    }
    const Int ub =
        _countOffsets[i] +
        std::ranges::count_if(staticInputVarNodes(),
                              [&](const std::shared_ptr<VarNode>& vNode) {
                                return vNode->inDomain(_cover[i]);
                              });
    if (!outputVarNode(i).constDomain()->contains(_countOffsets[i], ub)) {
      return true;
    }
  }
  return false;
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
      invariantGraph(), outputVarNode(0),
      std::vector<std::shared_ptr<VarNode>>(staticInputVarNodes()),
      _cover.front(), _countOffsets.front()));
  return true;
}

void GlobalCardinalityNode::registerOutputVars(propagation::SolverBase& solver,
                                               SolverMapping& mapping) const {
  for (size_t i = 0; i < _cover.size(); ++i) {
    assert(std::ranges::none_of(outputVarNodes().begin(),
                                outputVarNodes().begin() + static_cast<Int>(i),
                                [&](const std::shared_ptr<VarNode>& vNode) {
                                  return vNode.get() == &outputVarNode(i);
                                }));

    if (_countOffsets[i] == 0) {
      assert(mapping.solverId(outputVarNode(i)) == propagation::NULL_ID);
      makeSolverVar(outputVarNode(i), solver, mapping);
    } else {
      assert(mapping.solverId(outputVarNode(i)) == propagation::NULL_ID);
      mapping.setIntermediateId(
          ptrConst(), i,
          solver.makeIntVar(
              0, 0,
              std::max<Int>(0, static_cast<Int>(staticInputVarNodes().size()) -
                                   _countOffsets[i])));
      mapping.setSolverId(
          outputVarNode(i),
          solver.makeIntView<propagation::IntOffsetView>(
              solver, mapping.intermediateId(ptrConst(), i), _countOffsets[i]));
    }
  }
  assert(std::ranges::all_of(outputVarNodes().begin(), outputVarNodes().end(),
                             [&](const std::shared_ptr<VarNode>& vId) {
                               return mapping.solverId(vId) !=
                                      propagation::NULL_ID;
                             }));
}

void GlobalCardinalityNode::registerNode(propagation::SolverBase& solver,
                                         SolverMapping& mapping) const {
  std::vector<propagation::VarViewId> inputVarIds;
  std::ranges::transform(staticInputVarNodes(), std::back_inserter(inputVarIds),
                         [&](const auto& id) { return mapping.solverId(id); });

  std::vector<propagation::VarViewId> outputVarIds;
  outputVarIds.reserve(outputVarNodes().size());
  for (size_t i = 0; i < _cover.size(); ++i) {
    assert(mapping.intermediateId(ptrConst(), i) == propagation::NULL_ID ||
           mapping.intermediateId(ptrConst(), i).isVar());

    outputVarIds.emplace_back(mapping.intermediateId(ptrConst(), i) ==
                                      propagation::NULL_ID
                                  ? mapping.solverId(outputVarNode(i))
                                  : mapping.intermediateId(ptrConst(), i));
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
