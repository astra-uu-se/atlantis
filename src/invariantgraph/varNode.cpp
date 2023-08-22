#include "invariantgraph/varNode.hpp"

namespace invariantgraph {

static SearchDomain convertDomain(const VarNode::FZNVariable& variable) {
  if (std::holds_alternative<fznparser::BoolVar>(variable)) {
    const fznparser::BoolVar& boolVar = std::get<fznparser::BoolVar>(variable);
    std::vector<Int> boolDomain;
    if (boolVar.upperBound() == false) {
      boolDomain.emplace_back(0);
    }
    if (boolVar.lowerBound() == false) {
      boolDomain.emplace_back(1);
    }
    return SearchDomain(std::move(boolDomain));
  }
  const fznparser::IntSet& intDomain =
      std::get<fznparser::IntVar>(variable).domain();
  if (intDomain.isInterval()) {
    return SearchDomain(intDomain.lowerBound(), intDomain.upperBound());
  }
  return SearchDomain(intDomain.elements());
}

VarNode::VarNode(VarNodeId id, const VarNode::FZNVariable& variable)
    : _id(id),
      _variable(variable),
      _domain{std::move(convertDomain(*_variable))},
      _isIntVar(std::holds_alternative<fznparser::IntVar>(variable)) {}

VarNode::VarNode(VarNodeId id, SearchDomain&& domain, bool isIntVar)
    : _id(id),
      _variable(std::nullopt),
      _domain(std::move(domain)),
      _isIntVar(isIntVar) {}

VarNode::VarNode(VarNodeId id, const SearchDomain& domain, bool isIntVar)
    : _id(id), _variable(std::nullopt), _domain(domain), _isIntVar(isIntVar) {}

VarNodeId VarNode::varNodeId() const noexcept { return _id; }

std::optional<FZNVariable> VarNode::variable() const { return _variable; }

std::string_view VarNode::identifier() const {
  if (!_variable.holds_value()) {
    return std::string_view::empty();
  }
  if (std::holds_alternative<fznparser::BoolVar>(*_variable)) {
    return std::get<fznparser::BoolVar>(*_variable).identifier();
  }
  return std::get<fznparser::IntVar>(*_variable).identifier();
}

VarId VarNode::varId() const { return _varId; }

void setVarId(VarId varId) {
  assert(_varId == NULL_ID);
  _varId = varId;
}

SearchDomain& VarNode::domain() noexcept { return _domain; }

bool VarNode::isFixed() const noexcept { return _domain.isFixed(); }

inline bool VarNode::isIntVar() const noexcept { return _isIntVar; }

VarId VarNode::postDomainConstraint(Engine& engine,
                                    std::vector<DomainEntry>&& domain) {
  if (domain.size() == 0 || _domainViolationId != NULL_ID) {
    return _domainViolationId;
  }
  const size_t interval =
      domain.back().upperBound - domain.front().lowerBound + 1;

  // domain.size() - 1 = number of "holes" in the domain:
  const float denseness = 1.0 - ((float)(domain.size() - 1) / (float)interval);
  if (SPARSE_MIN_DENSENESS <= denseness) {
    _domainViolationId = engine.makeIntView<InSparseDomain>(
        engine, this->varId(), std::move(domain));
  } else {
    _domainViolationId =
        engine.makeIntView<InDomain>(engine, this->varId(), std::move(domain));
  }
  return _domainViolationId;
}

std::vector<DomainEntry> VarNode::constrainedDomain(const Engine& engine) {
  assert(this->varId() != NULL_ID);
  const VarId varId = this->varId();
  return _domain.relativeComplementIfIntersects(engine.lowerBound(varId),
                                                engine.upperBound(varId));
}

std::pair<Int, Int> VarNode::bounds() const noexcept {
  return _domain.bounds();
}

const std::vector<InvariantNodeId>& VarNode::inputFor() const noexcept {
  return _inputFor;
}

const std::vector<InvariantNodeId>& VarNode::staticInputFor() const noexcept {
  return _staticInputFor;
}

const std::vector<InvariantNodeId>& VarNode::dynamicInputFor() const noexcept {
  return _dynamicInputFor;
}

const std::unordered_set<InvariantNodeId, InvariantNodeIdHash>&
VarNode::definingNodes() const noexcept {
  return _definingNodes;
}

InvariantNodeId VarNode::definedBy() const noexcept {
  if (_definingNodes.size() == 1) {
    return _definingNodes.begin();
  }
}

void VarNode::markAsInputFor(InvariantNodeId listeningInvNodeId,
                             bool isStaticInput) {
  _inputFor.push_back(listeningInvNodeId);
  if (isStaticInput) {
    _staticInputFor.push_back(listeningInvNodeId);
  } else {
    _dynamicInputFor.push_back(listeningInvNodeId);
  }
}

void VarNode::unmarkAsDefinedBy(InvariantNodeId definingInvNodeId) {
  _definingNodes.erase(definingInvNodeId);
}

void VarNode::unmarkAsInputFor(InvariantNodeId listeningInvNodeId,
                               bool isStaticInput) {
  if (isStaticInput) {
    auto it = _staticInputFor.begin();
    while (it != _staticInputFor.end()) {
      if (*it == listeningInvNodeId) {
        it = _staticInputFor.erase(it);
      } else {
        it++;
      }
    }
  } else {
    auto it = _dynamicInputFor.begin();
    while (it != _dynamicInputFor.end()) {
      if (*it == listeningInvNodeId) {
        it = _dynamicInputFor.erase(it);
      } else {
        it++;
      }
    }
  }
}

void VarNode::markOutputTo(InvariantNodeId definingInvNodeId) {
  assert(definingInvNodeId != NULL_NODE_ID);
  _definingNodes.emplace(definingInvariant);
}

std::optional<Int> VarNode::constantValue() const noexcept {
  auto [lb, ub] = _domain.bounds();
  return lb == ub ? std::optional<Int>{lb} : std::optional<Int>{};
}

}  // namespace invariantgraph