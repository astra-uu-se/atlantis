#include "atlantis/invariantgraph/invariantGraph.hpp"

#include <numeric>
#include <queue>
#include <ranges>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "atlantis/invariantgraph/gecodeSolver.hpp"
#include "atlantis/invariantgraph/invariantGraphRoot.hpp"
#include "atlantis/invariantgraph/varNode.hpp"
#include "atlantis/invariantgraph/violationInvariantNode.hpp"
#include "atlantis/invariantgraph/violationInvariantNodes/boolAllEqualNode.hpp"
#include "atlantis/invariantgraph/violationInvariantNodes/intAllEqualNode.hpp"
#include "atlantis/propagation/invariants/linear.hpp"
#include "atlantis/propagation/solverBase.hpp"
#include "atlantis/search/neighborhoods/neighborhoodCombinator.hpp"
#include "atlantis/search/searchVariable.hpp"
#include "atlantis/utils/domains.hpp"
#include "atlantis/utils/fznAst.hpp"
#include "atlantis/utils/graph.hpp"

using atlantis::propagation::SolverBase;

namespace atlantis::invariantgraph {

InvariantGraphRoot& InvariantGraph::root() const {
  return dynamic_cast<InvariantGraphRoot&>(*_implicitConstraintNodes.front());
}

InvariantGraph::InvariantGraph(const bool breakDynamicCycles)
    : _varNodes{std::make_shared<VarNode>(
                    false, std::make_shared<SearchDomain>(std::vector<Int>{1})),
                std::make_shared<VarNode>(false, std::make_shared<SearchDomain>(
                                                     std::vector<Int>{0}))},
      _boolVarNodes{_varNodes[0], _varNodes[1]},
      _constraintSolver(std::make_shared<GecodeSolver>()),
      _breakDynamicCycles(breakDynamicCycles),
      _objectiveVarNode{nullptr} {
  for (const auto& bVar : _boolVarNodes) {
    bVar->setConstraintVarId(
        _constraintSolver->newBoolVar(bVar->inDomain(true)));
  }
  addImplicitConstraintNode(std::make_shared<InvariantGraphRoot>(*this));
}

ConstraintSolver& InvariantGraph::constraintSolver() {
  return *_constraintSolver;
}

const ConstraintSolver& InvariantGraph::constraintSolverConst() const {
  return *_constraintSolver;
}

bool InvariantGraph::containsVarNode(const std::string& identifier) const {
  return !_namedVarNodeIndices.empty() &&
         _namedVarNodeIndices.contains(identifier);
}

bool InvariantGraph::containsVarNode(const Int i) const {
  return !_intVarNodeIndices.empty() && _intVarNodeIndices.contains(i);
}

bool InvariantGraph::containsVarNode(bool) const { return true; }

VarNode& InvariantGraph::retrieveBoolVarNode(const bool b) {
  assert(_boolVarNodes.at(0)->inDomain(bool{false}));
  assert(_boolVarNodes.at(1)->inDomain(bool{true}));
  return *_boolVarNodes.at(b ? 1 : 0);
}

VarNode& InvariantGraph::retrieveBoolVarNode(const bool value,
                                             const bool forceNewVar) {
  if (!forceNewVar) {
    return retrieveBoolVarNode(value);
  }
  return *_varNodes.emplace_back(std::make_shared<VarNode>(
      false, std::make_shared<SearchDomain>(std::vector<Int>{value ? 0 : 1}),
      _constraintSolver->newBoolVar(value), DomainType::DOM_FIXED));
}

VarNode InvariantGraph::retrieveBoolVarNode(const std::string& identifier,
                                            const DomainType domainType) {
  if (!containsVarNode(identifier)) {
    const std::shared_ptr<VarNode> varNode =
        _varNodes.emplace_back(std::make_shared<VarNode>(
            identifier, false, _constraintSolver->newBoolVar(), domainType));
    _namedVarNodeIndices.emplace(identifier, varNode);
    return *varNode;
  }
  assert(!varNode(identifier).isIntVar());
  return *_namedVarNodeIndices.at(identifier);
}

VarNode& InvariantGraph::retrieveBoolVarNode(const DomainType domainType) {
  return *_varNodes.emplace_back(std::make_shared<VarNode>(
      false, _constraintSolver->newBoolVar(), domainType));
}

VarNode& InvariantGraph::retrieveBoolVarNode(const bool b,
                                             const std::string& identifier) {
  auto& varNode = retrieveBoolVarNode(b);
  if (!containsVarNode(identifier)) {
    _namedVarNodeIndices.emplace(identifier, varNode.ptr());
  } else {
    if (varNode.isIntVar()) {
      throw std::invalid_argument("Variable " + identifier +
                                  " is not a boolean variable");
    }
    if (!varNode.isFixed() || !varNode.inDomain(b)) {
      throw std::invalid_argument("Variable " + identifier +
                                  " is not fixed to " + (b ? "true" : "false"));
    }
  }
  return varNode;
}

VarNode& InvariantGraph::retrieveBoolVarNode(
    const std::shared_ptr<SearchDomain>& domain, const DomainType domainType) {
  if (domain->isFixed()) {
    return retrieveBoolVarNode(domain->lowerBound() == 0);
  }
  return *_varNodes.emplace_back(std::make_shared<VarNode>(
      false, domain, _constraintSolver->newBoolVar(), domainType));
}

VarNode& InvariantGraph::retrieveIntVarNode(const Int value) {
  if (!containsVarNode(value)) {
    const auto& varNode = _varNodes.emplace_back(std::make_shared<VarNode>(
        true, std::make_shared<SearchDomain>(std::vector<Int>{value}),
        _constraintSolver->newIntVar(value), DomainType::DOM_FIXED));
    _intVarNodeIndices.emplace(value, varNode);
    return *varNode;
  }
  const auto& var = varNode(value);
  if (!var.isIntVar()) {
    throw std::invalid_argument("Variable " + std::to_string(value) +
                                " is not an integer variable");
  }
  if (!var.isFixed() || !var.inDomain(value)) {
    throw std::invalid_argument("Variable " + std::to_string(value) +
                                " is not fixed to " + std::to_string(value));
  }
  return *_intVarNodeIndices.at(value);
}

VarNode& InvariantGraph::retrieveIntVarNode(const Int value,
                                            const bool forceNewVar) {
  if (!forceNewVar) {
    retrieveIntVarNode(value);
  }
  const auto& varNode = _varNodes.emplace_back(std::make_shared<VarNode>(
      true, std::make_shared<SearchDomain>(std::vector<Int>{value}),
      _constraintSolver->newIntVar(value), DomainType::DOM_FIXED));
  if (!containsVarNode(value)) {
    _intVarNodeIndices.emplace(value, varNode);
  }
  return *varNode;
}

VarNode InvariantGraph::retrieveIntVarNode(const std::string& identifier) {
  if (!containsVarNode(identifier)) {
    throw std::invalid_argument("No variable with identifier " + identifier);
  }
  if (!varNode(identifier).isIntVar()) {
    throw std::invalid_argument("Variable " + identifier +
                                " is not an integer variable");
  }
  return *_namedVarNodeIndices.at(identifier);
}

VarNode& InvariantGraph::retrieveIntVarNode(const Int i,
                                            const std::string& identifier) {
  auto& varNode = retrieveIntVarNode(i);
  if (!containsVarNode(identifier)) {
    const auto [pair, success] =
        _namedVarNodeIndices.emplace(identifier, varNode.ptr());
    assert(success);
    return *pair->second;
  }
  return *_namedVarNodeIndices.at(identifier);
}

VarNode& InvariantGraph::retrieveIntVarNode(
    const std::shared_ptr<SearchDomain>& domain, const DomainType domainType) {
  if (domain->isFixed()) {
    return retrieveIntVarNode(domain->lowerBound());
  }
  return *_varNodes.emplace_back(std::make_shared<VarNode>(
      true, domain, _constraintSolver->newIntVar(*domain), domainType));
}

VarNode& InvariantGraph::retrieveIntVarNode(
    const std::shared_ptr<SearchDomain>& domain) {
  return retrieveIntVarNode(domain, DomainType::DOM_DOMAIN);
}

VarNode& InvariantGraph::retrieveIntVarNode(
    const std::shared_ptr<SearchDomain>& domain, const std::string& identifier,
    const DomainType domainType) {
  if (containsVarNode(identifier)) {
    auto& node = varNode(identifier);
    if (!node.isIntVar()) {
      throw std::invalid_argument("Variable " + identifier +
                                  " is not an integer variable");
    }
    return node;
  }

  auto& varNode = domain->isFixed()
                      ? retrieveIntVarNode(domain->lowerBound())
                      : *_varNodes.emplace_back(std::make_shared<VarNode>(
                            identifier, true, domain,
                            _constraintSolver->newIntVar(*domain), domainType));

  assert(!containsVarNode(identifier));
  _namedVarNodeIndices.emplace(identifier, varNode.ptr());
  return varNode;
}

VarNode& InvariantGraph::retrieveIntVarNode(
    const std::shared_ptr<SearchDomain>& dom, const std::string& identifier) {
  return retrieveIntVarNode(dom, identifier, DomainType::DOM_DOMAIN);
}

void InvariantGraph::setObjective(const std::shared_ptr<VarNode>& objNode,
                                  const ObjectiveDirection objDir) {
  assert((objNode == nullptr) == (objDir == ObjectiveDirection::NONE));
  _objectiveVarNode = objNode;
  _objectiveDirection = objDir;
}

VarNode& InvariantGraph::varNode(const std::string& identifier) {
  assert(_namedVarNodeIndices.contains(identifier));
  return *_namedVarNodeIndices.at(identifier);
}

VarNode& InvariantGraph::varNode(const Int value) {
  assert(_intVarNodeIndices.contains(value));
  return *_intVarNodeIndices.at(value);
}

void InvariantGraph::replaceInvariantNodes() {
  size_t invIndex = 0;

  for (; invIndex < _invariantNodes.size(); ++invIndex) {
    auto& invNode = *_invariantNodes.at(invIndex);
    invNode.updateState();
    if (invNode.state() == InvariantNodeState::SUBSUMED) {
      invNode.deactivate();
    } else if (invNode.canBeReplaced()) {
      const bool wasReplaced = invNode.replace();
      assert(wasReplaced);
      if (wasReplaced) {
        invNode.deactivate();
      }
    }
  }

  for (size_t implIndex = 0; implIndex < _implicitConstraintNodes.size();
       ++implIndex) {
    auto& implNode = *_implicitConstraintNodes.at(implIndex);
    implNode.updateState();
    if (implNode.state() == InvariantNodeState::SUBSUMED) {
      implNode.deactivate();
    } else if (implNode.canBeReplaced()) {
      if (implNode.replace()) {
        implNode.deactivate();
      }
      // Only implicit constraints were added by the replace call:
      assert(invIndex == _invariantNodes.size());
    }
  }
}

void InvariantGraph::deactivateUnusedInvariantNodes() {
  std::unordered_set<std::shared_ptr<VarNode>> onStack;
  onStack.reserve(_varNodes.size());
  std::vector<std::shared_ptr<VarNode>> unusedOutputVars;
  unusedOutputVars.reserve(_varNodes.size());

  // Find all non-objective output vars
  for (size_t i = 0; i < _varNodes.size(); ++i) {
    if (_varNodes[i] == _objectiveVarNode || _varNodes[i]->isOutputVar() ||
        !_varNodes[i]->staticInputTo().empty() ||
        !_varNodes[i]->dynamicInputTo().empty() ||
        _varNodes[i]->definingNodes().size() != 1) {
      continue;
    }
    unusedOutputVars.emplace_back(_varNodes[i]);
    onStack.emplace(_varNodes[i]);
  }

  std::unordered_set<std::shared_ptr<InvariantNode>> modifiedInvariantNodes;
  modifiedInvariantNodes.reserve(_varNodes.size());

  while (!unusedOutputVars.empty()) {
    const std::shared_ptr<VarNode> outputNode = unusedOutputVars.back();
    unusedOutputVars.pop_back();
    onStack.erase(outputNode);

    // Note that if the variable is defined by multiple invariants, then it will
    // be duplicated and an all equal will be posted, making it constrained.
    if (outputNode == _objectiveVarNode || outputNode->isOutputVar() ||
        !outputNode->staticInputTo().empty() ||
        !outputNode->dynamicInputTo().empty() ||
        outputNode->definingNodes().size() != 1) {
      continue;
    }

    const std::shared_ptr<InvariantNode>& defInv =
        *outputNode->definingNodes().begin();

    const std::shared_ptr<ImplicitConstraintNode> impl =
        std::dynamic_pointer_cast<ImplicitConstraintNode>(defInv);

    if (defInv != nullptr || defInv->constrainsOutput(*outputNode)) {
      continue;
    }

    assert(defInv->state() == InvariantNodeState::ACTIVE);

    defInv->removeOutputVarNode(*outputNode);
    if (defInv->state() != InvariantNodeState::SUBSUMED) {
      modifiedInvariantNodes.emplace(defInv);
      continue;
    }

    for (const auto& staticInput : defInv->staticInputVarNodes()) {
      if (!onStack.contains(staticInput)) {
        onStack.emplace(staticInput);
        unusedOutputVars.emplace_back(staticInput);
      }
    }
    for (const auto& dynamicInput : defInv->dynamicInputVarNodes()) {
      if (!onStack.contains(dynamicInput)) {
        onStack.emplace(dynamicInput);
        unusedOutputVars.emplace_back(dynamicInput);
      }
    }
    defInv->deactivate();
    modifiedInvariantNodes.erase(defInv);
  }

  // The modified invariants must be updated and potentially replaced.
  size_t invIndex = _invariantNodes.size();
  for (const auto& invNode : modifiedInvariantNodes) {
    invNode->updateState();
    if (invNode->state() == InvariantNodeState::SUBSUMED) {
      invNode->deactivate();
    } else if (invNode->canBeReplaced()) {
      [[maybe_unused]] const bool wasReplaced = invNode->replace();
      assert(wasReplaced);
      invNode->deactivate();
    }
  }
  for (; invIndex < _invariantNodes.size(); ++invIndex) {
    _invariantNodes[invIndex]->updateState();
    if (_invariantNodes[invIndex]->state() == InvariantNodeState::SUBSUMED) {
      _invariantNodes[invIndex]->deactivate();
    } else if (_invariantNodes[invIndex]->canBeReplaced()) {
      [[maybe_unused]] const bool wasReplaced =
          _invariantNodes[invIndex]->replace();
      assert(wasReplaced);
      _invariantNodes[invIndex]->deactivate();
    }
  }
}

void InvariantGraph::makeImplicitConstraintNodes() {
  std::vector<std::pair<size_t, std::pair<size_t, size_t>>> implicitNodeRanks;
  implicitNodeRanks.reserve(_invariantNodes.size());

  size_t invIndex = 0;
  for (; invIndex < _invariantNodes.size(); ++invIndex) {
    auto& invNode = *_invariantNodes.at(invIndex);
    if (invNode.state() == InvariantNodeState::ACTIVE &&
        invNode.canBeMadeImplicit()) {
      implicitNodeRanks.emplace_back(invIndex, invNode.implicitRank());
      assert(implicitNodeRanks.back().second.first != 0);
      assert(implicitNodeRanks.back().second.second != 0);
    }
  }

  // Sort descending on ranks:
  std::ranges::sort(implicitNodeRanks,
                    [](const std::pair<size_t, std::pair<size_t, size_t>>& a,
                       const std::pair<size_t, std::pair<size_t, size_t>>& b) {
                      return a.second > b.second;
                    });

  size_t implIndex = _implicitConstraintNodes.size();

  for (const size_t index : std::views::keys(implicitNodeRanks)) {
    auto& invNode = *_invariantNodes.at(index);
    assert(invNode.state() == InvariantNodeState::ACTIVE);
    if (invNode.canBeMadeImplicit()) {
      const bool wasMadeImpicit = invNode.makeImplicit();
      assert(wasMadeImpicit);
      assert(invIndex == _invariantNodes.size());
      invNode.deactivate();
    }
  }

  // Iterate over the new implicit constraints:
  for (; implIndex < _implicitConstraintNodes.size(); ++implIndex) {
    auto& implNode = *_implicitConstraintNodes.at(implIndex);
    implNode.updateState();
    if (implNode.state() == InvariantNodeState::SUBSUMED) {
      implNode.deactivate();
    } else if (implNode.canBeReplaced()) {
      if (implNode.replace()) {
        implNode.deactivate();
      }
      // Only implicit constraints were added by the replace call:
      assert(invIndex == _invariantNodes.size());
    }
  }
}

void InvariantGraph::replaceFixedVars() {
  // replace all fixed input variables:
  for (auto& vNode : _varNodes) {
    if (vNode->definingNodes().empty() &&
        (!vNode->staticInputTo().empty() || !vNode->dynamicInputTo().empty()) &&
        vNode->isFixed()) {
      if (vNode->isIntVar()) {
        if (!_intVarNodeIndices.contains(vNode->lowerBound())) {
          _intVarNodeIndices.emplace(vNode->lowerBound(), vNode);
        } else if (_intVarNodeIndices.at(vNode->lowerBound()) != vNode) {
          replaceVarNode(vNode, _intVarNodeIndices.at(vNode->lowerBound()));
        }
      } else {
        const bool val = vNode->inDomain(bool{true});
        const size_t index = val ? 1 : 0;
        assert(_boolVarNodes[index]->inDomain(val));
        if (_boolVarNodes[index] != vNode) {
          replaceVarNode(vNode, _boolVarNodes[index]);
        }
      }
    }
  }
}

const VarNode& InvariantGraph::varNodeConst(
    const std::string& identifier) const {
  assert(_namedVarNodeIndices.contains(identifier));
  return *_namedVarNodeIndices.at(identifier);
}

const std::vector<std::shared_ptr<ImplicitConstraintNode>>&
InvariantGraph::implicitConstraintNodes() const {
  return _implicitConstraintNodes;
}

std::shared_ptr<InvariantNode> InvariantGraph::addInvariantNode(
    std::shared_ptr<InvariantNode>&& node) {
  if (node->state() == InvariantNodeState::SUBSUMED) {
    return nullptr;
  }
  if (node->state() != InvariantNodeState::UNINITIALIZED) {
    throw InvariantGraphException(
        "InvariantGraph::addInvariantNode: invariant: \"" +
        node->dotLangIdentifier() + "\" already initialized.");
  }
  const auto& invNode = _invariantNodes.emplace_back(std::move(node));
  invNode->init();
  invNode->postConstraint();
  return _invariantNodes.back();
}

void InvariantGraph::replaceVarNode(std::shared_ptr<VarNode> oldNode,
                                    std::shared_ptr<VarNode> newNode) {
  if (oldNode == newNode) {
    return;
  }
  newNode->domain()->removeAllValuesExcept(*oldNode->constDomain());
  while (!oldNode->definingNodes().empty()) {
    const auto& invNode = *(oldNode->definingNodes().begin());
    invNode->replaceDefinedVar(*oldNode, newNode);
    assert(!oldNode->definingNodes().contains(invNode));
    assert(newNode->definingNodes().contains(invNode));
    assert(std::ranges::none_of(invNode->outputVarNodes(),
                                [&](const std::shared_ptr<VarNode>& other) {
                                  return other == oldNode;
                                }));
    assert(std::ranges::any_of(invNode->outputVarNodes(),
                               [&](const std::shared_ptr<VarNode>& other) {
                                 return other == newNode;
                               }));
  }
  assert(oldNode->definingNodes().empty());

  while (!oldNode->staticInputTo().empty()) {
    [[maybe_unused]] const auto& invNode = oldNode->staticInputTo().front();
    invNode->replaceStaticInputVarNode(*oldNode, newNode);
    assert(
        std::ranges::none_of(oldNode->staticInputTo(),
                             [&](const std::shared_ptr<InvariantNode>& other) {
                               return other == invNode;
                             }));
    assert(
        std::ranges::any_of(newNode->staticInputTo(),
                            [&](const std::shared_ptr<InvariantNode>& other) {
                              return other == invNode;
                            }));
    assert(std::ranges::none_of(invNode->staticInputVarNodes(),
                                [&](const std::shared_ptr<VarNode>& other) {
                                  return other == oldNode;
                                }));
    assert(std::ranges::any_of(invNode->staticInputVarNodes(),
                               [&](const std::shared_ptr<VarNode>& other) {
                                 return other == newNode;
                               }));
  }
  assert(oldNode->staticInputTo().empty());

  while (!oldNode->dynamicInputTo().empty()) {
    [[maybe_unused]] const auto& invNode = oldNode->dynamicInputTo().front();
    invNode->replaceStaticInputVarNode(*oldNode, newNode);
    assert(
        std::ranges::none_of(oldNode->dynamicInputTo(),
                             [&](const std::shared_ptr<InvariantNode>& other) {
                               return other == invNode;
                             }));
    assert(
        std::ranges::any_of(newNode->dynamicInputTo(),
                            [&](const std::shared_ptr<InvariantNode>& other) {
                              return other == invNode;
                            }));
    assert(std::ranges::none_of(invNode->dynamicInputVarNodes(),
                                [&](const std::shared_ptr<VarNode>& other) {
                                  return other == oldNode;
                                }));
    assert(std::ranges::any_of(invNode->dynamicInputVarNodes(),
                               [&](const std::shared_ptr<VarNode>& other) {
                                 return other == newNode;
                               }));
  }
  assert(oldNode->staticInputTo().empty());
  assert(oldNode->dynamicInputTo().empty());

  if (oldNode->isFixed()) {
    if (oldNode->isIntVar()) {
      if (_intVarNodeIndices.contains(oldNode->lowerBound()) &&
          _intVarNodeIndices.at(oldNode->lowerBound()) == oldNode) {
        _intVarNodeIndices.erase(oldNode->lowerBound());
        _intVarNodeIndices.emplace(oldNode->lowerBound(), newNode);
      }
    } else {
      const bool val = oldNode->inDomain(bool{true});
      const size_t index = val ? 1 : 0;
      assert(_boolVarNodes.at(index)->inDomain(val));
      if (_boolVarNodes.at(index) == oldNode) {
        _boolVarNodes[index] = newNode;
      }
    }
  }
  for (auto& id : std::views::values(_namedVarNodeIndices)) {
    if (id == oldNode) {
      id = newNode;
    }
  }
  newNode->setIsOutputVar(newNode->isOutputVar() || oldNode->isOutputVar());
}

std::shared_ptr<ImplicitConstraintNode>
InvariantGraph::addImplicitConstraintNode(
    std::shared_ptr<ImplicitConstraintNode>&& node) {
  const auto& implNode = _implicitConstraintNodes.emplace_back(std::move(node));
  implNode->init();
  implNode->postConstraint();
  return _implicitConstraintNodes.back();
}

void InvariantGraph::createNeighborhood(SolverBase& solver,
                                        SolverMapping& mapping) const {
  if (mapping.hasGlobalNeighborhood()) {
    return;
  }
  std::vector<std::shared_ptr<search::neighborhoods::Neighborhood>>
      neighborhoods;
  neighborhoods.reserve(_implicitConstraintNodes.size());

  for (auto const& implicitConstraint : _implicitConstraintNodes) {
    if (!mapping.hasNeighborhood(implicitConstraint->mappingId())) {
      implicitConstraint->registerNode(solver, mapping);
    }
    if (mapping.hasNeighborhood(implicitConstraint->mappingId())) {
      neighborhoods.emplace_back(
          mapping.neighborhood(implicitConstraint->mappingId()));
    }
  }
  if (neighborhoods.size() == 1) {
    mapping.setGlobalNeighborhood(neighborhoods.front());
  } else {
    mapping.setGlobalNeighborhood(
        std::make_shared<search::neighborhoods::NeighborhoodCombinator>(
            std::move(neighborhoods)));
  }
}

const VarNode& InvariantGraph::objectiveVarNode() const {
  return *_objectiveVarNode;
}

void InvariantGraph::populateRootNode() {
  for (auto& vNode : _varNodes) {
    if (vNode->definingNodes().empty() &&
        (!vNode->staticInputTo().empty() || !vNode->dynamicInputTo().empty() ||
         _objectiveVarNode == vNode) &&
        !vNode->isFixed()) {
      root().addSearchVarNode(vNode);
      assert(root().outputVarNodes().back() == vNode);
      assert(vNode->definingNodes().contains(root()));
    }
  }
}

void InvariantGraph::splitMultiDefinedVars() {
  // DO NOT emplace to _varNodes while doing this kind of iteration!

  size_t newSize = 0;
  for (const auto& vNode : _varNodes) {
    newSize += std::max<size_t>(
        1, vNode->definingNodes().size() + (vNode->isFixed() ? 1 : 0));
  }
  assert(_varNodes.size() <= newSize);
  _varNodes.reserve(newSize);

  const size_t end = _varNodes.size();

  for (size_t i = 0; i < end; i++) {
    const bool isFixed = _varNodes[i]->isFixed();
    const size_t numSplitNodes =
        _varNodes[i]->definingNodes().size() + (isFixed ? 1 : 0);
    if (numSplitNodes <= 1) {
      continue;
    }

    std::vector<std::shared_ptr<InvariantNode>> replacedDefiningNodes;
    replacedDefiningNodes.reserve(_varNodes[i]->definingNodes().size());

    bool isFirstInvNodeId = !isFixed;
    for (const auto& defInv : _varNodes[i]->definingNodes()) {
      if (isFirstInvNodeId) {
        isFirstInvNodeId = false;
      } else {
        replacedDefiningNodes.emplace_back(defInv);
      }
    }

    std::vector<std::shared_ptr<VarNode>> splitNodes;
    splitNodes.reserve(isFixed ? 0 : numSplitNodes);
    if (!isFixed) {
      splitNodes.emplace_back(_varNodes[i]);
    }

    for (const auto& invNode : replacedDefiningNodes) {
      const auto& splitVarNode =
          _varNodes.emplace_back(std::make_shared<VarNode>(
              _varNodes[i]->isIntVar(),
              std::make_shared<SearchDomain>(*(_varNodes[i]->constDomain())),
              _varNodes[i]->constraintVarId(),
              isFixed ? DomainType::DOM_FIXED : DomainType::DOM_NONE));

      invNode->replaceDefinedVar(*_varNodes[i], splitVarNode);

      if (!isFixed) {
        splitNodes.emplace_back(splitVarNode);
      }
    }

    assert(splitNodes.empty() == isFixed);

    if (!isFixed) {
      if (_varNodes[i]->isIntVar()) {
        addInvariantNode(std::make_shared<IntAllEqualNode>(
            *this, std::move(splitNodes), true, true));
      } else {
        addInvariantNode(std::make_shared<BoolAllEqualNode>(
            *this, std::move(splitNodes), true, true));
      }
    }
  }
  assert(_varNodes.size() == newSize);
}

void InvariantGraph::breakSelfCycles() {
  const size_t end = _invariantNodes.size();
  for (size_t i = 0; i < end; ++i) {
    const auto& invNode = _invariantNodes[i];
    std::unordered_set<VarNode*> visitedOutputs;
    visitedOutputs.reserve(invNode->outputVarNodes().size());
    for (const auto& outputVar : invNode->outputVarNodes()) {
      if (visitedOutputs.contains(outputVar.get())) {
        continue;
      }
      bool hasSelfCycle = false;
      for (size_t k = 0; k < (_breakDynamicCycles ? 2 : 1); ++k) {
        for (const auto& inputVar : k == 0 ? invNode->staticInputVarNodes()
                                           : invNode->dynamicInputVarNodes()) {
          if (outputVar == inputVar) {
            hasSelfCycle = true;
            break;
          }
        }
        if (hasSelfCycle) {
          break;
        }
      }
      if (hasSelfCycle) {
        const auto& newDefinedVar =
            _varNodes.emplace_back(std::make_shared<VarNode>(
                outputVar->isIntVar(),
                std::make_shared<SearchDomain>(outputVar->lowerBound(),
                                               outputVar->upperBound()),
                outputVar->constraintVarId(), DomainType::DOM_NONE));
        invNode->replaceDefinedVar(*outputVar, newDefinedVar);
        if (outputVar->isIntVar()) {
          addInvariantNode(std::make_shared<IntAllEqualNode>(
              *this, outputVar, newDefinedVar, true, true));
        } else {
          addInvariantNode(std::make_shared<BoolAllEqualNode>(
              *this, outputVar, newDefinedVar, true, true));
        }
        if (outputVar->definingNodes().empty()) {
          root().addSearchVarNode(outputVar);
        }
        visitedOutputs.emplace(newDefinedVar.get());
      }
    }
  }
}

void InvariantGraph::breakCycles() {
  std::unordered_map<VarNode*, size_t> varIndex(_varNodes.size());
  std::unordered_map<InvariantNode*, size_t> invIndex(_invariantNodes.size());
  for (size_t i = 0; i < _varNodes.size(); ++i) {
    varIndex.emplace(_varNodes[i].get(), i);
  }

  for (size_t i = 0; i < _invariantNodes.size(); ++i) {
    invIndex.emplace(_invariantNodes[i].get(), _varNodes.size() + i);
  }

  auto graph = Graph(_varNodes.size() + _invariantNodes.size());
  for (size_t i = 0; i < _varNodes.size(); ++i) {
    std::vector<size_t> outgoingStatic;
    outgoingStatic.reserve(_varNodes[i]->staticInputTo().size());
    std::vector<size_t> outgoingDynamic;
    outgoingDynamic.reserve(_varNodes[i]->dynamicInputTo().size());
    for (const auto& inv : _varNodes[i]->staticInputTo()) {
      const auto iter = invIndex.find(inv.get());
      if (iter != invIndex.end()) {
        outgoingStatic.emplace_back(iter->second);
      }
    }
    for (const auto& inv : _varNodes[i]->dynamicInputTo()) {
      const auto iter = invIndex.find(inv.get());
      if (iter != invIndex.end()) {
        outgoingDynamic.emplace_back(iter->second);
      }
    }
    const size_t priority = _varNodes[i]->isFixed()
                                ? (std::numeric_limits<size_t>::max() - 1)
                                : _varNodes[i]->domain()->size();
    graph.addNode(Graph::Node(i, priority, std::move(outgoingStatic),
                              std::move(outgoingDynamic)));
  }
  for (size_t i = 0; i < _invariantNodes.size(); ++i) {
    std::vector<size_t> outgoingStatic;
    outgoingStatic.reserve(_invariantNodes[i]->outputVarNodes().size());
    for (const auto& var : _invariantNodes[i]->outputVarNodes()) {
      const auto iter = varIndex.find(var.get());
      if (iter != varIndex.end()) {
        outgoingStatic.emplace_back(iter->second);
      }
    }
    graph.addNode(Graph::Node(i, std::numeric_limits<Int>::max(),
                              std::move(outgoingStatic)));
  }

  for (size_t i = 0; i < (_breakDynamicCycles ? 2 : 1); ++i) {
    const auto removedArcs =
        i == 0 ? graph.breakStaticCycles() : graph.breakDynamicCycles();

    for (const auto [origin, destination] : removedArcs) {
      assert(origin != destination);
      assert(origin < _varNodes.size());
      assert(destination >= _varNodes.size());
      const auto& pivotVar = _varNodes[origin];
      const auto& pivotInv = _invariantNodes[destination - _varNodes.size()];

      const auto& newDefinedVar =
          _varNodes.emplace_back(std::make_shared<VarNode>(
              pivotVar->isIntVar(),
              std::make_shared<SearchDomain>(pivotVar->lowerBound(),
                                             pivotVar->upperBound()),
              pivotVar->constraintVarId(), DomainType::DOM_NONE));
      pivotInv->replaceStaticInputVarNode(*pivotVar, newDefinedVar);
      pivotInv->replaceDynamicInputVarNode(*pivotVar, newDefinedVar);
      if (pivotVar->isIntVar()) {
        addInvariantNode(std::make_shared<IntAllEqualNode>(
            *this, pivotVar, newDefinedVar, true, true));
      } else {
        addInvariantNode(std::make_shared<BoolAllEqualNode>(
            *this, pivotVar, newDefinedVar, true, true));
      }
      assert(newDefinedVar->definingNodes().empty());
      root().addSearchVarNode(newDefinedVar);
    }
  }
}

void createVarsUtil(
    const InvariantGraph& graph, const std::shared_ptr<InvariantNode>& invNode,
    std::unordered_set<std::shared_ptr<InvariantNode>>& visitedInvNodes,
    std::unordered_set<std::shared_ptr<InvariantNode>>& onStack,
    SolverBase& solver, SolverMapping& mapping) {
  if (visitedInvNodes.contains(invNode)) {
    return;
  }
  visitedInvNodes.emplace(invNode);
  if (invNode->state() != InvariantNodeState::ACTIVE) {
    return;
  }
  onStack.emplace(invNode);
  for (const auto& inputVar : invNode->staticInputVarNodes()) {
    for (const auto& defInv : inputVar->definingNodes()) {
      assert(!onStack.contains(defInv));
      if (!visitedInvNodes.contains(defInv)) {
        createVarsUtil(graph, defInv, visitedInvNodes, onStack, solver,
                       mapping);
      }
    }
  }
  assert(std::ranges::none_of(
      invNode->staticInputVarNodes(),
      [&](const auto& inputVar) { return inputVar == nullptr; }));
  invNode->registerOutputVars(solver, mapping);
  onStack.erase(invNode);
}

void InvariantGraph::createVars(SolverBase& solver,
                                SolverMapping& mapping) const {
  // create a _solver var for each fixed boolean:
  for (const auto& vNode : _boolVarNodes) {
    assert(vNode->definingNodes().empty());
    if (vNode->staticInputTo().empty() && vNode->dynamicInputTo().empty()) {
      continue;
    }
    if (mapping.solverId(vNode->mappingId()) == propagation::NULL_ID) {
      assert(vNode->constantValue().has_value());
      const Int constant = *vNode->constantValue();
      mapping.setSolverId(vNode->mappingId(),
                          solver.makeIntVar(constant, constant, constant));
    }
  }

  // create a _solver var for each fixed integer var
  for (const auto& [constant, vNode] : _intVarNodeIndices) {
    assert(vNode->definingNodes().empty());
    if (vNode->staticInputTo().empty() && vNode->dynamicInputTo().empty()) {
      continue;
    }
    if (mapping.solverId(vNode->mappingId()) == propagation::NULL_ID) {
      assert(vNode->constantValue().has_value() &&
             vNode->constantValue().value() == constant);
      mapping.setSolverId(vNode->mappingId(),
                          solver.makeIntVar(constant, constant, constant));
    }
  }

  // create a _solver var for each other fixed variable:
  for (const auto& vNode : _varNodes) {
    if (!vNode->definingNodes().empty() ||
        (vNode->staticInputTo().empty() && vNode->dynamicInputTo().empty())) {
      continue;
    }
    // assert(vNode->isFixed());
    if (mapping.solverId(vNode->mappingId()) == propagation::NULL_ID) {
      mapping.setSolverId(
          vNode->mappingId(),
          solver.makeIntVar(vNode->lowerBound(), vNode->lowerBound(),
                            vNode->lowerBound()));
    }
  }

  std::unordered_set<std::shared_ptr<InvariantNode>> visitedInvNodes;
  std::unordered_set<std::shared_ptr<InvariantNode>> onStack;
  visitedInvNodes.reserve(_invariantNodes.size() +
                          _implicitConstraintNodes.size());
  onStack.reserve(_invariantNodes.size() + _implicitConstraintNodes.size());

  for (const auto& implNode : _implicitConstraintNodes) {
    if (implNode->state() == InvariantNodeState::ACTIVE) {
      createVarsUtil(*this, implNode, visitedInvNodes, onStack, solver,
                     mapping);
    }
  }

  for (const auto& invNode : _invariantNodes) {
    if (invNode->state() == InvariantNodeState::ACTIVE) {
      createVarsUtil(*this, invNode, visitedInvNodes, onStack, solver, mapping);
    }
  }
}

void InvariantGraph::createImplicitConstraints(SolverBase& solver,
                                               SolverMapping& mapping) const {
  for (const auto& implicitConstraintNode : _implicitConstraintNodes) {
    if (implicitConstraintNode->state() == InvariantNodeState::ACTIVE) {
      assert(std::ranges::all_of(implicitConstraintNode->outputVarNodes(),
                                 [&](const std::shared_ptr<VarNode>& varNode) {
                                   return mapping.solverId(
                                              varNode->mappingId()) !=
                                          propagation::NULL_ID;
                                 }));
      implicitConstraintNode->registerNode(solver, mapping);
    }
  }
}

void InvariantGraph::createInvariants(SolverBase& solver,
                                      SolverMapping& mapping) const {
  for (const auto& invariantNode : _invariantNodes) {
    if (invariantNode->state() == InvariantNodeState::ACTIVE) {
      assert(std::ranges::all_of(invariantNode->outputVarNodes(),
                                 [&](const std::shared_ptr<VarNode>& varNode) {
                                   return mapping.solverId(
                                              varNode->mappingId()) !=
                                          propagation::NULL_ID;
                                 }));
      invariantNode->registerNode(solver, mapping);
    }
  }
}

propagation::VarViewId InvariantGraph::createViolations(
    SolverBase& solver, SolverMapping& mapping) const {
  std::vector<propagation::VarViewId> violations;
  for (const auto& definingNode : _invariantNodes) {
    if (definingNode->state() == InvariantNodeState::ACTIVE &&
        !definingNode->isReified() &&
        definingNode->violationVarId(mapping) != propagation::NULL_ID) {
      violations.emplace_back(definingNode->violationVarId(mapping));
    }
  }

  for (auto& vNode : _varNodes) {
    if (mapping.solverId(vNode->mappingId()) != propagation::NULL_ID) {
      const propagation::VarViewId violationId =
          vNode->postDomainConstraint(solver, mapping);
      if (violationId != propagation::NULL_ID) {
        violations.emplace_back(violationId);
      }
    }
  }
  if (violations.empty()) {
    return propagation::VAR_VIEW_NULL_ID;
  }
  if (violations.size() == 1) {
    return violations.front();
  }
  const propagation::VarViewId totalViolation = solver.makeIntVar(0, 0, 0);
  solver.makeInvariant<propagation::Linear>(solver, totalViolation,
                                            std::move(violations));
  return totalViolation;
}

SolverMapping InvariantGraph::construct(SolverBase& solver) const {
  const bool wasClosed = !solver.isOpen();
  if (wasClosed) {
    solver.open();
  }
  SolverMapping mapping;
  createVars(solver, mapping);
  createImplicitConstraints(solver, mapping);
  createInvariants(solver, mapping);
  createNeighborhood(solver, mapping);
  solver.computeBounds();
  mapping.setTotalViolationId(createViolations(solver, mapping));
  mapping.setObjectiveDirection(_objectiveDirection);
  if (_objectiveDirection != ObjectiveDirection::NONE &&
      _objectiveVarNode != nullptr) {
    mapping.setObjectiveId(mapping.solverId(_objectiveVarNode->mappingId()));
    mapping.setObjectiveOptimalValue(_objectiveDirection ==
                                             ObjectiveDirection::MINIMIZE
                                         ? objectiveVarNode().lowerBound()
                                         : objectiveVarNode().upperBound());
  } else {
    mapping.setObjectiveOptimalValue(0);
  }

  if (wasClosed) {
    solver.close();
  }
  return mapping;
}

void InvariantGraph::open() { _isOpen = true; }

void InvariantGraph::close() {
  if (!isOpen()) {
    return;
  }
  _constraintSolver->fixPoint();
  sanity(false);
  updateDomains();
  sanity(false);
  replaceInvariantNodes();
  sanity(false);
  deactivateUnusedInvariantNodes();
  sanity(false);
  makeImplicitConstraintNodes();
  sanity(false);
  replaceFixedVars();
  sanity(false);
  populateRootNode();
  sanity(false);
  splitMultiDefinedVars();
  sanity(true);
  breakSelfCycles();
  sanity(true);
  breakCycles();
  sanity(true);
}

void InvariantGraph::updateDomains() {
  std::array<std::vector<std::shared_ptr<SearchDomain>>, 2> domains{
      std::vector<std::shared_ptr<SearchDomain>>(
          _constraintSolver->numIntVars(), nullptr),
      std::vector<std::shared_ptr<SearchDomain>>(
          _constraintSolver->numBoolVars(), nullptr)};
  for (size_t i = 0; i < _constraintSolver->numIntVars(); i++) {
    domains[0][i] = std::make_shared<SearchDomain>(
        _constraintSolver->intVarDomain({i, true}));
  }
  for (size_t i = 0; i < _constraintSolver->numBoolVars(); i++) {
    domains[1][i] = std::make_shared<SearchDomain>(
        _constraintSolver->boolVarDomain({i, false}));
  }

  for (const auto& varNode : _varNodes) {
    if (varNode->isIntVar()) {
      varNode->replaceDomain(domains[0][size_t{varNode->constraintVarId()}]);
    } else {
      varNode->replaceDomain(domains[1][size_t{varNode->constraintVarId()}]);
    }
  }
}

void InvariantGraph::sanity([[maybe_unused]] const bool oneDefInv) {
#ifndef NDEBUG
  assert(_boolVarNodes.at(0)->isFixed());
  assert(_boolVarNodes.at(0)->inDomain(bool{false}));
  assert(_boolVarNodes.at(1)->isFixed());
  assert(_boolVarNodes.at(1)->inDomain(bool{true}));
  for (const auto& [constant, vNode] : _intVarNodeIndices) {
    assert(vNode->isIntVar());
    assert(vNode->isFixed());
    assert(vNode->inDomain(constant));
  }
  for (const auto& vNode : _varNodes) {
    for (const auto& invNode : vNode->definingNodes()) {
      assert(std::ranges::any_of(invNode->outputVarNodes(),
                                 [&](const std::shared_ptr<VarNode>& other) {
                                   return other == vNode;
                                 }));
    }
    for (const auto& invNode : vNode->staticInputTo()) {
      assert(std::ranges::any_of(invNode->staticInputVarNodes(),
                                 [&](const std::shared_ptr<VarNode>& other) {
                                   return other == vNode;
                                 }));
    }
    for (const auto& invNode : vNode->dynamicInputTo()) {
      assert(std::ranges::any_of(invNode->dynamicInputVarNodes(),
                                 [&](const std::shared_ptr<VarNode>& other) {
                                   return other == vNode;
                                 }));
    }
    if (oneDefInv) {
      assert(vNode->definingNodes().size() <= 1);
    }
  }
  for (const auto& implNode : _implicitConstraintNodes) {
    for (const auto& vNode : implNode->outputVarNodes()) {
      assert(std::ranges::any_of(
          vNode->definingNodes(),
          [&](const auto& other) { return other == implNode; }));
    }
    assert(implNode->staticInputVarNodes().empty());
    assert(implNode->dynamicInputVarNodes().empty());
  }
  for (const auto& invNode : _invariantNodes) {
    for (const auto& vNode : invNode->outputVarNodes()) {
      assert(std::ranges::any_of(
          vNode->definingNodes(),
          [&](const auto& other) { return other == invNode; }));
    }
    for (const auto& vNode : invNode->staticInputVarNodes()) {
      assert(std::ranges::any_of(
          vNode->staticInputTo(),
          [&](const auto& other) { return other == invNode; }));
    }
    for (const auto& vNode : invNode->dynamicInputVarNodes()) {
      assert(std::ranges::any_of(
          vNode->dynamicInputTo(),
          [&](const auto& other) { return other == invNode; }));
    }
  }
#endif
}

void InvariantGraph::writeDotFile(std::ostream& o) const {
  std::unordered_set<std::shared_ptr<const VarNode>> visitedVarNodes(
      _varNodes.size() + 1);

  o << "digraph G {" << std::endl;

  std::vector<std::string> varIdentifiers;
  varIdentifiers.reserve(_namedVarNodeIndices.size());
  for (const auto& identifier : std::views::keys(_namedVarNodeIndices)) {
    varIdentifiers.push_back(identifier);
  }

  std::ranges::sort(varIdentifiers.begin(), varIdentifiers.end());

  for (const std::string& identifier : varIdentifiers) {
    const auto& vNode = varNodeConst(identifier);
    visitedVarNodes.emplace(vNode.constPtr());
    if (!vNode.staticInputTo().empty() || !vNode.dynamicInputTo().empty() ||
        !vNode.definingNodes().empty()) {
      vNode.dotLangIdentifier(o, identifier);
    }
  }

  size_t n = 1;

  for (auto& varNode : _varNodes) {
    if (visitedVarNodes.contains(varNode)) {
      continue;
    }
    if (varNode->staticInputTo().empty() || varNode->dynamicInputTo().empty() ||
        varNode->definingNodes().empty()) {
      continue;
    }
    const std::string identifier =
        varNode->isFixed()
            ? (varNode->isIntVar()
                   ? std::to_string(varNode->lowerBound())
                   : (varNode->inDomain(bool{false}) ? "false" : "true"))
            : ("ATLANTIS_" + std::to_string(n));
    varNode->dotLangIdentifier(o, identifier);
    if (!varNode->isFixed()) {
      ++n;
    }
  }

  for (const auto& impl : _implicitConstraintNodes) {
    if (impl->state() == InvariantNodeState::ACTIVE) {
      impl->dotLangEntry(o);
    }
  }

  for (const auto& inv : _invariantNodes) {
    if (inv->state() == InvariantNodeState::ACTIVE) {
      inv->dotLangEntry(o);
    }
  }

  for (const auto& impl : _implicitConstraintNodes) {
    if (impl->state() == InvariantNodeState::ACTIVE) {
      impl->dotLangEdges(o);
    }
  }

  for (const auto& inv : _invariantNodes) {
    if (inv->state() == InvariantNodeState::ACTIVE) {
      inv->dotLangEdges(o);
    }
  }

  o << '}' << std::endl;
}

}  // namespace atlantis::invariantgraph
