#include "atlantis/invariantgraph/varNode.hpp"

#include <cassert>
#include <fznparser/variables.hpp>

#include "atlantis/propagation/solverBase.hpp"
#include "atlantis/propagation/views/equalConst.hpp"
#include "atlantis/propagation/views/greaterEqualConst.hpp"
#include "atlantis/propagation/views/inDomain.hpp"
#include "atlantis/propagation/views/inIntervalConst.hpp"
#include "atlantis/propagation/views/inSparseDomain.hpp"
#include "atlantis/propagation/views/lessEqualConst.hpp"
#include "atlantis/propagation/views/notEqualConst.hpp"
#include "atlantis/search/searchVariable.hpp"
#include "atlantis/sortedUniqueVector.hpp"
#include "atlantis/utils/domains.hpp"

namespace atlantis::invariantgraph {

VarNode::VarNode(VarNodeId varNodeId, bool isIntVar, DomainType domainType)
    : _varNodeId(varNodeId),
      _isIntVar(isIntVar),
      _domainType(domainType),
      _domain(std::make_shared<SearchDomain>(0, 1)) {
  assert(!isIntVar);
}

VarNode::VarNode(VarNodeId varNodeId, bool isIntVar,
                 const std::shared_ptr<SearchDomain>& domain,
                 DomainType domainType)
    : _varNodeId(varNodeId),
      _isIntVar(isIntVar),
      _domainType(domainType),
      _domain(domain) {}

VarNodeId VarNode::varNodeId() const noexcept { return _varNodeId; }

propagation::VarViewId VarNode::varId() const { return _varId; }

void VarNode::setVarId(propagation::VarViewId varId) {
  assert(_varId == propagation::NULL_ID);
  _varId = varId;
}

std::shared_ptr<const SearchDomain> VarNode::constDomain() const noexcept {
  return _domain;
}

std::shared_ptr<SearchDomain> VarNode::domain() noexcept { return _domain; }

bool VarNode::isFixed() const noexcept {
  if (isIntVar()) {
    return _domain->isFixed();
  }
  return (_domain->lowerBound() == 0) == (_domain->upperBound() == 0);
}

bool VarNode::isIntVar() const noexcept { return _isIntVar; }

bool VarNode::isViolationVar() const noexcept { return _isViolationVar; }

void VarNode::setIsViolationVar(bool isViolVar) {
  if (_isIntVar && isViolVar) {
    throw std::runtime_error("Cannot set violation var on IntVar");
  }
  _isViolationVar = isViolVar;
}

propagation::VarViewId VarNode::postDomainConstraint(
    propagation::SolverBase& solver) {
  if (_domainViolationId != propagation::NULL_ID) {
    return _domainViolationId;
  }
  if (_domainType == DomainType::DOM_NONE ||
      ((staticInputTo().empty() || dynamicInputTo().empty()) &&
       definingNodes().empty())) {
    return propagation::VarViewId{propagation::NULL_ID};
  }
  if (_domainType == DomainType::DOM_FIXED && !isFixed()) {
    throw std::runtime_error("Domain type is fixed but domain is not fixed");
  }

  if (varId() == propagation::NULL_ID) {
    throw std::runtime_error("VarNode has no varId");
  }

  const Int solverLb = solver.lowerBound(varId());
  const Int solverUb = solver.upperBound(varId());

  if (!isIntVar()) {
    const bool holdsTrue = solverLb <= 0 && 0 <= solverUb;
    const bool holdsFalse = solverUb >= 1;
    if (!isFixed()) {
      return propagation::VarViewId{propagation::NULL_ID};
    }
    if ((inDomain(bool{true}) && !holdsTrue) ||
        (inDomain(bool{false}) && !holdsFalse)) {
      throw InconsistencyException(
          "VarNode::postDomainConstraint: Solver domain and invariant graph "
          "domain do not overlap");
    }
    if ((inDomain(bool{true}) && !holdsFalse) ||
        (inDomain(bool{false}) && !holdsTrue)) {
      return propagation::VarViewId{propagation::NULL_ID};
    }
    if (inDomain(bool{true})) {
      _domainViolationId =
          solver.makeIntView<propagation::EqualConst>(solver, varId(), 0);
    } else {
      _domainViolationId =
          solver.makeIntView<propagation::NotEqualConst>(solver, varId(), 0);
    }
    return _domainViolationId;
  }

  if (_domainType == DomainType::DOM_FIXED || _domain->isFixed()) {
    if (lowerBound() < solverLb || solverUb < lowerBound()) {
      throw std::runtime_error("Solver var domain range is " +
                               std::to_string(solverLb) + ".." +
                               std::to_string(solverUb) +
                               " but invariant graph var node is fixed to " +
                               std::to_string(lowerBound()));
    }
    if (solverLb != solverUb) {
      _domainViolationId = solver.makeIntView<propagation::EqualConst>(
          solver, varId(), lowerBound());
    }
    return _domainViolationId;
  }

  if (_domainType == DomainType::DOM_LOWER_BOUND) {
    if (solverUb < lowerBound()) {
      throw std::runtime_error(
          "Solver var max value is " + std::to_string(solverUb) +
          " but invariant graph var node is bounded below from " +
          std::to_string(lowerBound()));
    }
    if (solverLb < lowerBound()) {
      _domainViolationId = solver.makeIntView<propagation::GreaterEqualConst>(
          solver, varId(), lowerBound());
    }
    return _domainViolationId;
  }

  if (_domainType == DomainType::DOM_UPPER_BOUND) {
    if (solverLb > upperBound()) {
      throw std::runtime_error(
          "Solver var min value is " + std::to_string(solverLb) +
          " but invariant graph var node is bounded above from " +
          std::to_string(upperBound()));
    }
    if (solverUb > upperBound()) {
      _domainViolationId = solver.makeIntView<propagation::LessEqualConst>(
          solver, varId(), upperBound());
    }
    return _domainViolationId;
  }

  if (_domainType == DomainType::DOM_RANGE) {
    if (solverUb < lowerBound() || solverLb > upperBound()) {
      throw std::runtime_error(
          "Solver var domain range is " + std::to_string(solverLb) + ".." +
          std::to_string(solverUb) +
          " but invariant graph var node domain range is " +
          std::to_string(lowerBound()) + ".." + std::to_string(upperBound()));
    }
    if (lowerBound() < solverLb || solverUb < upperBound()) {
      _domainViolationId = solver.makeIntView<propagation::InIntervalConst>(
          solver, varId(), lowerBound(), upperBound());
    }
    return _domainViolationId;
  }
  assert(_domainType == DomainType::DOM_DOMAIN);

  std::vector<DomainEntry> domain =
      _domain->createDomainEntries(solverLb, solverUb);

  if (domain.empty()) {
    // The node domain contains the solver domain:
    assert(lowerBound() <= solverLb && solverUb <= upperBound());
    return _domainViolationId;
  }

  const size_t interval =
      domain.back().upperBound - domain.front().lowerBound + 1;

  // domain.size() - 1 = number of "holes" in the domain:
  if (domain.size() > 2 && interval < 1000) {
    _domainViolationId = solver.makeIntView<propagation::InSparseDomain>(
        solver, this->varId(), std::move(domain));
  } else {
    _domainViolationId = solver.makeIntView<propagation::InDomain>(
        solver, this->varId(), std::move(domain));
  }
  return _domainViolationId;
}

Int VarNode::lowerBound() const { return _domain->lowerBound(); }
Int VarNode::upperBound() const { return _domain->upperBound(); }

Int VarNode::val() const {
  if (_domain->isFixed()) {
    return _domain->lowerBound();
  }
  throw std::runtime_error("val() called on non-fixed var");
}

DomainType VarNode::domainType() const noexcept { return _domainType; }

void VarNode::tightenDomainType(DomainType domainType) {
  if ((domainType == DomainType::DOM_LOWER_BOUND &&
       _domainType == DomainType::DOM_UPPER_BOUND) ||
      (domainType == DomainType::DOM_UPPER_BOUND &&
       _domainType == DomainType::DOM_LOWER_BOUND)) {
    _domainType = DomainType::DOM_RANGE;
  }
  _domainType = std::max(_domainType, domainType);
}

void VarNode::setDomainType(DomainType domainType) { _domainType = domainType; }

bool VarNode::inDomain(Int val) const {
  if (!isIntVar()) {
    throw std::runtime_error("inDomain(Int) called on BoolVar");
  }
  return _domain->contains(val);
}

bool VarNode::inDomain(bool val) const {
  if (isIntVar()) {
    throw std::runtime_error("inDomain(bool) called on IntVar");
  }
  return val ? lowerBound() == 0 : upperBound() > 0;
}

void VarNode::removeValue(Int val, bool tightenDomainState) {
  if (!isIntVar()) {
    throw std::runtime_error("removeValue(Int) called on BoolVar");
  }
  const size_t prevSize = _domain->size();
  _domain->remove(val);
  if (tightenDomainState && prevSize != _domain->size()) {
    tightenDomainType(isFixed()            ? DomainType::DOM_FIXED
                      : val < lowerBound() ? DomainType::DOM_LOWER_BOUND
                      : val > upperBound() ? DomainType::DOM_UPPER_BOUND
                                           : DomainType::DOM_DOMAIN);
  }
}

void VarNode::removeValuesBelow(Int newLowerBound, bool tightenDomainState) {
  if (!isIntVar()) {
    throw std::runtime_error("removeValuesBelow(Int) called on BoolVar");
  }
  const size_t prevSize = _domain->size();
  _domain->removeBelow(newLowerBound);
  if (tightenDomainState && prevSize != _domain->size()) {
    tightenDomainType(DomainType::DOM_LOWER_BOUND);
  }
}

void VarNode::removeValuesAbove(Int newUpperBound, bool tightenDomainState) {
  if (!isIntVar()) {
    throw std::runtime_error("removeValuesAbove(Int) called on BoolVar");
  }
  const size_t prevSize = _domain->size();
  _domain->removeAbove(newUpperBound);
  if (tightenDomainState && prevSize != _domain->size()) {
    tightenDomainType(DomainType::DOM_LOWER_BOUND);
  }
}

void VarNode::removeValues(const SortedUniqueVector& values, bool tightenDomainState) {
  if (!isIntVar()) {
    throw std::runtime_error(
        "removeValues(const std::vector<Int>&) called on BoolVar");
  }
  if ((*values).empty()) {
    return;
  }
  if ((*values).size() == 1) {
    return removeValue((*values).front(), tightenDomainState);
  }
  const size_t prevSize = _domain->size();
  _domain->remove(values);
  if (tightenDomainState && prevSize != _domain->size()) {
    tightenDomainType(DomainType::DOM_DOMAIN);
  }
}

void VarNode::removeAllValuesExcept(const SortedUniqueVector& values, bool tightenDomainState) {
  if (!isIntVar()) {
    throw std::runtime_error(
        "removeValues(const std::vector<Int>&) called on BoolVar");
  }
  if ((*values).size() == 1) {
    return fixToValue((*values).front(), tightenDomainState);
  }
  const size_t prevSize = _domain->size();
  _domain->removeAllValuesExcept(values);
  if (tightenDomainState && prevSize != _domain->size()) {
    tightenDomainType(DomainType::DOM_DOMAIN);
  }
}

void VarNode::fixToValue(Int val, bool tightenDomainState) {
  if (!isIntVar()) {
    throw std::runtime_error("fixToValue(Int) called on BoolVar");
  }
  _domain->fix(val);
  if (tightenDomainState) {
    tightenDomainType(DomainType::DOM_FIXED);
  }
}

void VarNode::removeValue(bool val) {
  return fixToValue(!val);
}

void VarNode::fixToValue(bool val) {
  if (isIntVar()) {
    throw std::runtime_error("fixToValue(bool) called on IntVar");
  }
  _domain->fix(val ? 0 : 1);
  tightenDomainType(DomainType::DOM_FIXED);
}

std::vector<DomainEntry> VarNode::constrainedDomain(Int lb, Int ub) const {
  return _domain->createDomainEntries(lb, ub);
}

std::pair<Int, Int> VarNode::bounds() const { return _domain->bounds(); }

const std::vector<InvariantNodeId>& VarNode::staticInputTo() const noexcept {
  return _staticInputTo;
}

const std::vector<InvariantNodeId>& VarNode::dynamicInputTo() const noexcept {
  return _dynamicInputTo;
}

const std::unordered_set<InvariantNodeId, InvariantNodeIdHash>&
VarNode::definingNodes() const noexcept {
  return _outputOf;
}

InvariantNodeId VarNode::outputOf() const {
  if (_outputOf.empty()) {
    return InvariantNodeId{NULL_NODE_ID};
  }
  if (_outputOf.size() != 1) {
    throw std::runtime_error("VarNode is not an output var");
  }
  return *_outputOf.begin();
}

void VarNode::markAsInputFor(InvariantNodeId listeningInvNodeId,
                             bool isStaticInput) {
  if (isStaticInput) {
    _staticInputTo.emplace_back(listeningInvNodeId);
  } else {
    _dynamicInputTo.emplace_back(listeningInvNodeId);
  }
}

void VarNode::unmarkOutputTo(InvariantNodeId definingInvNodeId) {
  _outputOf.erase(definingInvNodeId);
}

void VarNode::unmarkAsInputFor(InvariantNodeId listeningInvariant,
                               bool isStaticInput) {
  if (isStaticInput) {
    for (Int i = static_cast<Int>(_staticInputTo.size()) - 1; i >= 0; --i) {
      if (_staticInputTo[i] == listeningInvariant) {
        _staticInputTo.erase(_staticInputTo.begin() + i);
      }
    }
  } else {
    for (Int i = static_cast<Int>(_dynamicInputTo.size()) - 1; i >= 0; --i) {
      if (_dynamicInputTo[i] == listeningInvariant) {
        _dynamicInputTo.erase(_dynamicInputTo.begin() + i);
      }
    }
  }
}

void VarNode::markOutputTo(InvariantNodeId definingInvariant) {
  assert(definingInvariant != NULL_NODE_ID);
  _outputOf.emplace(definingInvariant);
}

std::optional<Int> VarNode::constantValue() const noexcept {
  auto [lb, ub] = _domain->bounds();
  return lb == ub ? std::optional<Int>{lb} : std::optional<Int>{};
}

std::ostream& VarNode::dotLangIdentifier(std::ostream& o,
                                         const std::string& identifier) const {
  o << _varNodeId;
  return o << " [label=\"" << identifier << "\"];" << std::endl;
}

}  // namespace atlantis::invariantgraph
