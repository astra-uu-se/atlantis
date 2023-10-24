#include "invariantgraph/varNode.hpp"

#include "invariantgraph/invariantGraph.hpp"
#include "invariantgraph/invariantNode.hpp"

namespace atlantis::invariantgraph {

static SearchDomain convertDomain(const VarNode::FZNVar& var) {
  if (std::holds_alternative<fznparser::BoolVar>(var)) {
    const fznparser::BoolVar& boolVar = std::get<fznparser::BoolVar>(var);
    std::vector<Int> boolDomain;
    if (boolVar.upperBound() == true) {
      // 0 indicates there is no violation
      boolDomain.emplace_back(0);
    }
    if (boolVar.lowerBound() == false) {
      // non-zero indicates there is a violation
      boolDomain.emplace_back(1);
    }
    return SearchDomain(std::move(boolDomain));
  }
  const fznparser::IntSet& intDomain =
      std::get<fznparser::IntVar>(var).domain();
  if (intDomain.isInterval()) {
    return SearchDomain(intDomain.lowerBound(), intDomain.upperBound());
  }
  return SearchDomain(intDomain.elements());
}

VarNode::VarNode(VarNodeId varNodeId, const VarNode::FZNVar& var)
    : _varNodeId(varNodeId),
      _var(var),
      _domain{std::move(convertDomain(*_var))},
      _isIntVar(std::holds_alternative<fznparser::IntVar>(var)) {}

VarNode::VarNode(VarNodeId varNodeId, SearchDomain&& domain, bool isIntVar)
    : _varNodeId(varNodeId),
      _var(std::nullopt),
      _domain(std::move(domain)),
      _isIntVar(isIntVar) {}

VarNode::VarNode(VarNodeId varNodeId, const SearchDomain& domain, bool isIntVar)
    : _varNodeId(varNodeId),
      _var(std::nullopt),
      _domain(domain),
      _isIntVar(isIntVar) {}

VarNodeId VarNode::varNodeId() const noexcept { return _varNodeId; }

std::optional<VarNode::FZNVar> VarNode::var() const { return _var; }

std::string VarNode::identifier() const {
  if (!_var.has_value()) {
    return "";
  }
  if (std::holds_alternative<fznparser::BoolVar>(*_var)) {
    return std::get<fznparser::BoolVar>(*_var).identifier();
  }
  return std::get<fznparser::IntVar>(*_var).identifier();
}

propagation::VarId VarNode::varId() const { return _varId; }

void VarNode::setVarId(propagation::VarId varId) {
  assert(_varId == propagation::NULL_ID);
  _varId = varId;
}

const SearchDomain& VarNode::constDomain() const noexcept { return _domain; }

SearchDomain& VarNode::domain() noexcept { return _domain; }

bool VarNode::isFixed() const noexcept { return _domain.isFixed(); }

bool VarNode::isIntVar() const noexcept { return _isIntVar; }

propagation::VarId VarNode::postDomainConstraint(
    propagation::SolverBase& solver, std::vector<DomainEntry>&& domain) {
  if (domain.size() == 0 || _domainViolationId != propagation::NULL_ID) {
    return _domainViolationId;
  }
  const size_t interval =
      domain.back().upperBound - domain.front().lowerBound + 1;

  // domain.size() - 1 = number of "holes" in the domain:
  const float denseness = 1.0 - ((float)(domain.size() - 1) / (float)interval);
  if (SPARSE_MIN_DENSENESS <= denseness) {
    _domainViolationId = solver.makeIntView<propagation::InSparseDomain>(
        solver, this->varId(), std::move(domain));
  } else {
    _domainViolationId = solver.makeIntView<propagation::InDomain>(
        solver, this->varId(), std::move(domain));
  }
  return _domainViolationId;
}

Int VarNode::val() const {
  if (_domain.isFixed()) {
    return _domain.lowerBound();
  } else {
    throw std::runtime_error("val() called on non-fixed var");
  }
}

void VarNode::removeValue(Int val) {
  if (!isIntVar()) {
    throw std::runtime_error("removeValue(Int) called on BoolVar");
  }
  _domain.removeValue(val);
}

void VarNode::removeValue(bool val) {
  if (isIntVar()) {
    throw std::runtime_error("removeValue(bool) called on IntVar");
  }
  _domain.fix(val ? 0 : 1);
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

InvariantNodeId VarNode::outputOf() const noexcept {
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