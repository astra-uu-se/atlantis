#include "invariantgraph/varNode.hpp"

namespace atlantis::invariantgraph {

VarNode::VarNode(VarNodeId varNodeId, SearchDomain&& domain, bool isIntVar,
                 const std::string& identifier)
    : _varNodeId(varNodeId),
      _domain(std::move(domain)),
      _isIntVar(isIntVar),
      _identifier(identifier) {}

VarNode::VarNode(VarNodeId varNodeId, const SearchDomain& domain, bool isIntVar,
                 const std::string& identifier)
    : _varNodeId(varNodeId),
      _domain(domain),
      _isIntVar(isIntVar),
      _identifier(identifier) {}

VarNodeId VarNode::varNodeId() const noexcept { return _varNodeId; }

const std::string& VarNode::identifier() const { return _identifier; }

propagation::VarId VarNode::varId() const { return _varId; }

void VarNode::setVarId(propagation::VarId varId) {
  assert(_varId == propagation::NULL_ID);
  _varId = varId;
}

const SearchDomain& VarNode::constDomain() const noexcept { return _domain; }

SearchDomain& VarNode::domain() noexcept { return _domain; }

bool VarNode::isFixed() const noexcept {
  if (isIntVar()) {
    return _domain.isFixed();
  }
  return (_domain.lowerBound() == 0) == (_domain.upperBound() == 0);
}

bool VarNode::isIntVar() const noexcept { return _isIntVar; }

propagation::VarId VarNode::postDomainConstraint(
    propagation::SolverBase& solver, std::vector<DomainEntry>&& domain) {
  if (domain.size() == 0 || _domainViolationId != propagation::NULL_ID) {
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

bool VarNode::shouldEnforceDomain() noexcept { return _shouldEnforceDomain; }
bool VarNode::shouldEnforceDomain(bool b) noexcept {
  _shouldEnforceDomain = b;
  return _shouldEnforceDomain;
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
  return val ? _domain.lowerBound() == 0 : _domain.upperBound() > 0;
}

void VarNode::removeValue(Int val) {
  if (!isIntVar()) {
    throw std::runtime_error("removeValue(Int) called on BoolVar");
  }
  _domain.remove(val);
}

void VarNode::removeValuesBelow(Int newLowerBound) {
  if (!isIntVar()) {
    throw std::runtime_error("removeValuesBelow(Int) called on BoolVar");
  }
  _domain.removeBelow(newLowerBound);
}

void VarNode::removeValuesAbove(Int newUpperBound) {
  if (!isIntVar()) {
    throw std::runtime_error("removeValuesAbove(Int) called on BoolVar");
  }
  _domain.removeAbove(newUpperBound);
}

void VarNode::removeValues(const std::vector<Int>& values) {
  if (!isIntVar()) {
    throw std::runtime_error(
        "removeValues(const std::vector<Int>&) called on BoolVar");
  }
  _domain.remove(values);
}

void VarNode::removeAllValuesExcept(const std::vector<Int>& values) {
  if (!isIntVar()) {
    throw std::runtime_error(
        "removeValues(const std::vector<Int>&) called on BoolVar");
  }
  _domain.intersectWith(values);
}

void VarNode::fixValue(Int val) {
  if (!isIntVar()) {
    throw std::runtime_error("fixValue(Int) called on BoolVar");
  }
  _domain.fix(val);
}

void VarNode::removeValue(bool val) {
  if (isIntVar()) {
    throw std::runtime_error("removeValue(bool) called on IntVar");
  }
  _domain.fix(val ? 0 : 1);
}

void VarNode::fixValue(bool val) {
  if (!isIntVar()) {
    throw std::runtime_error("fixValue(bool) called on IntVar");
  }
  _domain.fix(val ? 1 : 0);
}

std::vector<DomainEntry> VarNode::constrainedDomain(
    const propagation::SolverBase& solver) {
  assert(this->varId() != propagation::NULL_ID);
  const propagation::VarId varId = this->varId();
  return _domain.relativeComplementIfIntersects(solver.lowerBound(varId),
                                                solver.upperBound(varId));
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
  if (_outputOf.size() == 0) {
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