#include "atlantis/invariantgraph/varNode.hpp"

#include <cassert>
#include <fznparser/variables.hpp>
#include <limits>

#include "atlantis/propagation/views/equalConst.hpp"
#include "atlantis/propagation/views/greaterEqualConst.hpp"
#include "atlantis/propagation/views/inDomain.hpp"
#include "atlantis/propagation/views/inIntervalConst.hpp"
#include "atlantis/propagation/views/inSparseDomain.hpp"
#include "atlantis/propagation/views/lessEqualConst.hpp"
#include "atlantis/search/neighbourhoods/neighbourhood.hpp"
#include "atlantis/search/searchVariable.hpp"
#include "atlantis/utils/variant.hpp"

namespace atlantis::invariantgraph {

VarNode::VarNode(VarNodeId varNodeId, bool isIntVar,
                 VarNode::DomainType domainType)
    : _varNodeId(varNodeId),
      _isIntVar(isIntVar),
      _domainType(domainType),
      _domain(0, 1) {
  assert(!isIntVar);
}

VarNode::VarNode(VarNodeId varNodeId, bool isIntVar, SearchDomain&& domain,
                 VarNode::DomainType domainType)
    : _varNodeId(varNodeId),
      _isIntVar(isIntVar),
      _domainType(domainType),
      _domain(std::move(domain)) {}

VarNodeId VarNode::varNodeId() const noexcept { return _varNodeId; }

propagation::VarId VarNode::varId() const { return _varId; }

void VarNode::setVarId(propagation::VarId varId) {
  assert(_varId == propagation::NULL_ID);
  _varId = varId;
}

const SearchDomain& VarNode::constDomain() const noexcept { return _domain; }

SearchDomain& VarNode::domain() noexcept { return _domain; }
const SearchDomain& VarNode::domainConst() const noexcept { return _domain; }

bool VarNode::isFixed() const noexcept {
  if (isIntVar()) {
    return _domain.isFixed();
  }
  return (_domain.lowerBound() == 0) == (_domain.upperBound() == 0);
}

bool VarNode::isIntVar() const noexcept { return _isIntVar; }

propagation::VarId VarNode::postDomainConstraint(
    propagation::SolverBase& solver) {
  if (_domainViolationId != propagation::NULL_ID) {
    return _domainViolationId;
  }
  if (_domainType == DomainType::NONE ||
      (inputTo().empty() && definingNodes().empty())) {
    return propagation::NULL_ID;
  }
  if (_domainType == DomainType::FIXED && !isFixed()) {
    throw std::runtime_error("Domain type is fixed but domain is not fixed");
  }

  if (varId() == propagation::NULL_ID) {
    throw std::runtime_error("VarNode has no varId");
  }

  const Int solverLb = solver.lowerBound(varId());
  const Int solverUb = solver.upperBound(varId());

  if (_domainType == DomainType::FIXED || _domain.isFixed()) {
    if (lowerBound() < solverLb || solverUb < lowerBound()) {
      throw std::runtime_error("Solver var domain range is" +
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

  if (_domainType == DomainType::LOWER_BOUND) {
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

  if (_domainType == DomainType::UPPER_BOUND) {
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

  if (_domainType == DomainType::RANGE) {
    if (solverUb < lowerBound() || solverLb > upperBound()) {
      throw std::runtime_error(
          "Solver var domain range is" + std::to_string(solverLb) + ".." +
          std::to_string(solverUb) +
          " but invariant graph var node domain range is " +
          std::to_string(lowerBound()) + ".." + std::to_string(upperBound()));
    }
    if (lowerBound() < solverLb || solverUb < upperBound()) {
      solver.makeIntView<propagation::InIntervalConst>(
          solver, varId(), lowerBound(), upperBound());
    }
    return _domainViolationId;
  }
  assert(_domainType == DomainType::DOMAIN);

  std::vector<DomainEntry> domain =
      _domain.relativeComplementIfIntersects(solverLb, solverUb);

  if (domain.empty()) {
    assert(solverLb <= lowerBound() && solverUb <= upperBound());
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

Int VarNode::lowerBound() const { return _domain.lowerBound(); }
Int VarNode::upperBound() const { return _domain.upperBound(); }

Int VarNode::val() const {
  if (_domain.isFixed()) {
    return _domain.lowerBound();
  } else {
    throw std::runtime_error("val() called on non-fixed var");
  }
}

VarNode::DomainType VarNode::domainType() const noexcept { return _domainType; }

void VarNode::tightenDomainType(VarNode::DomainType domainType) {
  if ((domainType == DomainType::LOWER_BOUND &&
       _domainType == DomainType::UPPER_BOUND) ||
      (domainType == DomainType::UPPER_BOUND &&
       _domainType == DomainType::LOWER_BOUND)) {
    _domainType = DomainType::RANGE;
  }
  _domainType = std::max(_domainType, domainType);
}

void VarNode::setDomainType(VarNode::DomainType domainType) {
  _domainType = domainType;
}

bool VarNode::inDomain(Int val) const {
  if (!isIntVar()) {
    throw std::runtime_error("inDomain(Int) called on BoolVar");
  }
  return _domain.contains(val);
}

bool VarNode::inDomain(bool val) const {
  if (!isIntVar()) {
    throw std::runtime_error("inDomain(Int) called on BoolVar");
  }
  return val ? lowerBound() == 0 : upperBound() > 0;
}

size_t VarNode::removeValue(Int val) {
  if (!isIntVar()) {
    throw std::runtime_error("removeValue(Int) called on BoolVar");
  }
  return _domain.remove(val);
}

size_t VarNode::removeValuesBelow(Int newLowerBound) {
  if (!isIntVar()) {
    throw std::runtime_error("removeValuesBelow(Int) called on BoolVar");
  }
  return _domain.removeBelow(newLowerBound);
}

size_t VarNode::removeValuesAbove(Int newUpperBound) {
  if (!isIntVar()) {
    throw std::runtime_error("removeValuesAbove(Int) called on BoolVar");
  }
  return _domain.removeAbove(newUpperBound);
}

size_t VarNode::removeValues(const std::vector<Int>& values) {
  if (!isIntVar()) {
    throw std::runtime_error(
        "removeValues(const std::vector<Int>&) called on BoolVar");
  }
  if (values.empty()) {
    return 0;
  }
  return _domain.remove(values);
}

size_t VarNode::removeAllValuesExcept(const std::vector<Int>& values) {
  if (!isIntVar()) {
    throw std::runtime_error(
        "removeValues(const std::vector<Int>&) called on BoolVar");
  }
  if (values.empty()) {
    throw std::runtime_error("removeAllValuesExcept called with empty values");
  }
  return _domain.intersectWith(values);
}

size_t VarNode::fixToValue(Int val) {
  if (!isIntVar()) {
    throw std::runtime_error("fixToValue(Int) called on BoolVar");
  }
  return _domain.fix(val);
}

size_t VarNode::removeValue(bool val) {
  if (isIntVar()) {
    throw std::runtime_error("removeValue(bool) called on IntVar");
  }
  return _domain.fix(val ? 0 : 1);
}

size_t VarNode::fixToValue(bool val) {
  if (isIntVar()) {
    throw std::runtime_error("fixToValue(bool) called on IntVar");
  }
  return _domain.fix(val ? 0 : 1);
}

std::vector<DomainEntry> VarNode::constrainedDomain(Int lb, Int ub) {
  return _domain.relativeComplementIfIntersects(lb, ub);
}

std::pair<Int, Int> VarNode::bounds() const { return _domain.bounds(); }

const std::vector<InvariantNodeId>& VarNode::inputTo() const noexcept {
  return _inputTo;
}

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
    return InvariantNodeId(NULL_NODE_ID);
  } else if (_outputOf.size() != 1) {
    throw std::runtime_error("VarNode is not an output var");
  }
  return *_outputOf.begin();
}

void VarNode::markAsInputFor(InvariantNodeId listeningInvNodeId,
                             bool isStaticInput) {
  _inputTo.push_back(listeningInvNodeId);
  if (isStaticInput) {
    _staticInputTo.push_back(listeningInvNodeId);
  } else {
    _dynamicInputTo.push_back(listeningInvNodeId);
  }
}

void VarNode::unmarkOutputTo(InvariantNodeId definingInvNodeId) {
  _outputOf.erase(definingInvNodeId);
}

void VarNode::unmarkAsInputFor(InvariantNodeId listeningInvNodeId,
                               bool isStaticInput) {
  if (isStaticInput) {
    auto it = _staticInputTo.begin();
    while (it != _staticInputTo.end()) {
      if (*it == listeningInvNodeId) {
        it = _staticInputTo.erase(it);
      } else {
        it++;
      }
    }
  } else {
    auto it = _dynamicInputTo.begin();
    while (it != _dynamicInputTo.end()) {
      if (*it == listeningInvNodeId) {
        it = _dynamicInputTo.erase(it);
      } else {
        it++;
      }
    }
  }
}

void VarNode::markOutputTo(InvariantNodeId definingInvNodeId) {
  assert(definingInvNodeId != NULL_NODE_ID);
  _outputOf.emplace(definingInvNodeId);
}

std::optional<Int> VarNode::constantValue() const noexcept {
  auto [lb, ub] = _domain.bounds();
  return lb == ub ? std::optional<Int>{lb} : std::optional<Int>{};
}

}  // namespace atlantis::invariantgraph
