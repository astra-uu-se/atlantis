#include "atlantis/invariantgraph/varNode.hpp"

#include <atlantis/invariantgraph/solverMapping.hpp>
#include <cassert>
#include <fznparser/variables.hpp>

#include "atlantis/invariantgraph/invariantNode.hpp"
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
class InvariantNode;

VarNode::VarNode(const std::string& identifier,
                 const bool isIntVar,
                 const std::shared_ptr<SearchDomain>& domain,
                 const ConstraintVarId constraintVarId,
                 const DomainType domainType)
    : _domainType(domainType),
      _isIntVar(isIntVar),
      _constraintSolverId(constraintVarId),
      _domain(domain),
      _identifier(identifier) {
  assert(_domain != nullptr);
}

std::shared_ptr<VarNode> VarNode::ptr() {
  return shared_from_this();
}
std::shared_ptr<const VarNode> VarNode::ptrConst() const {
  return shared_from_this();
}

VarNode::VarNode(const std::string& identifier,
                 const bool isIntVar, const ConstraintVarId constraintVarId,
                 const DomainType domainType)
    : _domainType(domainType),
      _isIntVar(isIntVar),
      _constraintSolverId(constraintVarId),
      _domain(std::make_shared<SearchDomain>(0, 1)),
      _identifier(identifier) {
  assert(!isIntVar);
}

VarNode::VarNode(const bool isIntVar,
                 const ConstraintVarId constraintVarId,
                 const DomainType domainType)
    : _domainType(domainType),
      _isIntVar(isIntVar),
      _constraintSolverId(constraintVarId),
      _domain(std::make_shared<SearchDomain>(0, 1)),
      _identifier(std::nullopt) {
  assert(!isIntVar);
}

VarNode::VarNode(const bool isIntVar,
                 const std::shared_ptr<SearchDomain>& domain,
                 const ConstraintVarId constraintVarId,
                 const DomainType domainType)
    : _domainType(domainType),
      _isIntVar(isIntVar),
      _constraintSolverId(constraintVarId),
      _domain(domain),
      _identifier(std::nullopt) {
  assert(_domain != nullptr);
}

ConstraintVarId VarNode::constraintVarId() const noexcept {
  return _constraintSolverId;
}

void VarNode::setConstraintVarId(const ConstraintVarId constraintVarId) {
  assert(_constraintSolverId == NULL_NODE_ID);
  _constraintSolverId = constraintVarId;
}

void VarNode::replaceDomain(const std::shared_ptr<SearchDomain>& newDomain) {
  _domain = newDomain;
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

void VarNode::setIsViolationVar(const bool isViolVar) {
  if (_isIntVar && isViolVar) {
    throw std::runtime_error("Cannot set violation var on IntVar");
  }
  _isViolationVar = isViolVar;
}

bool VarNode::isOutputVar() const noexcept { return _isOutputVar; }

void VarNode::setIsOutputVar(const bool isOutputVar) {
  _isOutputVar = isOutputVar;
}

propagation::VarViewId VarNode::postDomainConstraint(
    propagation::SolverBase& solver, SolverMapping& mapping) const {
  if (mapping.domainViolationId(ptrConst()) != propagation::NULL_ID) {
    return mapping.domainViolationId(ptrConst());
  }
  if (_domainType == DomainType::DOM_NONE ||
      ((staticInputTo().empty() || dynamicInputTo().empty()) &&
       definingNodes().empty())) {
    return propagation::VAR_VIEW_NULL_ID;
  }
  if (_domainType == DomainType::DOM_FIXED && !isFixed()) {
    throw std::runtime_error("Domain type is fixed but domain is not fixed");
  }

  if (mapping.solverId(ptrConst()) == propagation::NULL_ID) {
    throw std::runtime_error("VarNode has no varId");
  }

  const Int solverLb = solver.lowerBound(mapping.solverId(ptrConst()));
  const Int solverUb = solver.upperBound(mapping.solverId(ptrConst()));

  if (!isIntVar()) {
    const bool holdsTrue = solverLb <= 0 && 0 <= solverUb;
    const bool holdsFalse = solverUb >= 1;
    if (!isFixed()) {
      return propagation::VAR_VIEW_NULL_ID;
    }
    if ((inDomain(bool{true}) && !holdsFalse) ||
        (inDomain(bool{false}) && !holdsTrue)) {
      return propagation::VAR_VIEW_NULL_ID;
    }
    if (inDomain(bool{true})) {
      mapping.setDomainViolationId(
          ptrConst(), solver.makeIntView<propagation::EqualConst>(
                           solver, mapping.solverId(ptrConst()), 0));
    } else {
      mapping.setDomainViolationId(
          ptrConst(), solver.makeIntView<propagation::NotEqualConst>(
                           solver, mapping.solverId(ptrConst()), 0));
    }
    return mapping.domainViolationId(ptrConst());
  }

  if (_domainType == DomainType::DOM_FIXED || _domain->isFixed()) {
    if (solverLb == lowerBound() && solverUb == lowerBound()) {
      return mapping.domainViolationId(ptrConst());
    }
    mapping.setDomainViolationId(
        ptrConst(), solver.makeIntView<propagation::EqualConst>(
                         solver, mapping.solverId(ptrConst()), lowerBound()));
    return mapping.domainViolationId(ptrConst());
  }

  if (_domainType == DomainType::DOM_LOWER_BOUND) {
    if (solverLb >= lowerBound()) {
      return mapping.domainViolationId(ptrConst());
    }
    mapping.setDomainViolationId(
        ptrConst(), solver.makeIntView<propagation::GreaterEqualConst>(
                         solver, mapping.solverId(ptrConst()), lowerBound()));
    return mapping.domainViolationId(ptrConst());
  }

  if (_domainType == DomainType::DOM_UPPER_BOUND) {
    if (solverUb <= upperBound()) {
      return mapping.domainViolationId(ptrConst());
    }
    mapping.setDomainViolationId(
        ptrConst(), solver.makeIntView<propagation::LessEqualConst>(
                         solver, mapping.solverId(ptrConst()), upperBound()));
    return mapping.domainViolationId(ptrConst());
  }

  if (_domainType == DomainType::DOM_RANGE) {
    if (lowerBound() <= solverLb && solverUb <= upperBound()) {
      return mapping.domainViolationId(ptrConst());
    }
    mapping.setDomainViolationId(
        ptrConst(),
        solver.makeIntView<propagation::InIntervalConst>(
            solver, mapping.solverId(ptrConst()), lowerBound(), upperBound()));
    return mapping.domainViolationId(ptrConst());
  }
  assert(_domainType == DomainType::DOM_DOMAIN);

  if (_domain->contains(solverLb, solverUb)) {
    return mapping.domainViolationId(ptrConst());
  }

  if (_domain->isInterval()) {
    mapping.setDomainViolationId(
        ptrConst(),
        solver.makeIntView<propagation::InIntervalConst>(
            solver, mapping.solverId(ptrConst()), lowerBound(), upperBound()));
    return mapping.domainViolationId(ptrConst());
  }

  std::vector<DomainEntry> domain =
      _domain->createDomainEntries(lowerBound(), upperBound());
  assert(!domain.empty());

  const size_t interval =
      domain.back().upperBound - domain.front().lowerBound + 1;

  // domain.size() - 1 = number of "holes" in the domain:
  if (domain.size() > 2 && interval < 1000) {
    mapping.setDomainViolationId(
        ptrConst(),
        solver.makeIntView<propagation::InSparseDomain>(
            solver, mapping.solverId(ptrConst()), std::move(domain)));
  } else {
    mapping.setDomainViolationId(
        ptrConst(),
        solver.makeIntView<propagation::InDomain>(
            solver, mapping.solverId(ptrConst()), std::move(domain)));
  }
  return mapping.domainViolationId(ptrConst());
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

void VarNode::tightenDomainType() {
  tightenDomainType(_domain->isFixed()
                        ? DomainType::DOM_FIXED
                        : (_domain->isInterval() ? DomainType::DOM_RANGE
                                                 : DomainType::DOM_DOMAIN));
}

void VarNode::tightenDomainType(const DomainType domainType) {
  if ((domainType == DomainType::DOM_LOWER_BOUND &&
       _domainType == DomainType::DOM_UPPER_BOUND) ||
      (domainType == DomainType::DOM_UPPER_BOUND &&
       _domainType == DomainType::DOM_LOWER_BOUND)) {
    _domainType = DomainType::DOM_RANGE;
  }
  _domainType = std::max(_domainType, domainType);
}

void VarNode::setDomainType(const DomainType domainType) {
  _domainType = domainType;
}

bool VarNode::inDomain(const Int val) const {
  if (!isIntVar()) {
    throw std::runtime_error("inDomain(Int) called on BoolVar");
  }
  return _domain->contains(val);
}

bool VarNode::inDomain(const bool val) const {
  if (isIntVar()) {
    throw std::runtime_error("inDomain(bool) called on IntVar");
  }
  return val ? lowerBound() == 0 : upperBound() > 0;
}

void VarNode::removeValue(const Int val, const bool tightenDomainState) {
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

void VarNode::removeValuesBelow(const Int newLowerBound,
                                const bool tightenDomainState) {
  if (!isIntVar()) {
    throw std::runtime_error("removeValuesBelow(Int) called on BoolVar");
  }
  const size_t prevSize = _domain->size();
  _domain->removeBelow(newLowerBound);
  if (tightenDomainState && prevSize != _domain->size()) {
    tightenDomainType(DomainType::DOM_LOWER_BOUND);
  }
}

void VarNode::removeValuesAbove(const Int newUpperBound,
                                const bool tightenDomainState) {
  if (!isIntVar()) {
    throw std::runtime_error("removeValuesAbove(Int) called on BoolVar");
  }
  const size_t prevSize = _domain->size();
  _domain->removeAbove(newUpperBound);
  if (tightenDomainState && prevSize != _domain->size()) {
    tightenDomainType(DomainType::DOM_LOWER_BOUND);
  }
}

void VarNode::removeValues(const SortedUniqueVector& values,
                           const bool tightenDomainState) {
  if (!isIntVar()) {
    throw std::runtime_error(
        "removeValues(const std::vector<Int>&) called on BoolVar");
  }
  if (values->empty()) {
    return;
  }
  if (values->size() == 1) {
    return removeValue(values->front(), tightenDomainState);
  }
  const size_t prevSize = _domain->size();
  _domain->remove(values);
  if (tightenDomainState && prevSize != _domain->size()) {
    tightenDomainType(DomainType::DOM_DOMAIN);
  }
}

void VarNode::removeAllValuesExcept(const SortedUniqueVector& values,
                                    const bool tightenDomainState) {
  if (!isIntVar()) {
    throw std::runtime_error(
        "removeValues(const std::vector<Int>&) called on BoolVar");
  }
  if (values->size() == 1) {
    return fixToValue(values->front(), tightenDomainState);
  }
  const size_t prevSize = _domain->size();
  _domain->removeAllValuesExcept(values);
  if (tightenDomainState && prevSize != _domain->size()) {
    tightenDomainType(DomainType::DOM_DOMAIN);
  }
}

void VarNode::fixToValue(const Int val, const bool tightenDomainState) {
  if (!isIntVar()) {
    throw std::runtime_error("fixToValue(Int) called on BoolVar");
  }
  _domain->fix(val);
  if (tightenDomainState) {
    tightenDomainType(DomainType::DOM_FIXED);
  }
}

void VarNode::removeValue(const bool val) { return fixToValue(!val); }

void VarNode::fixToValue(const bool val) {
  if (isIntVar()) {
    throw std::runtime_error("fixToValue(bool) called on IntVar");
  }
  _domain->fix(val ? 0 : 1);
  tightenDomainType(DomainType::DOM_FIXED);
}

std::vector<DomainEntry> VarNode::constrainedDomain(const Int lb,
                                                    const Int ub) const {
  return _domain->createDomainEntries(lb, ub);
}

std::pair<Int, Int> VarNode::bounds() const { return _domain->bounds(); }

const std::vector<std::shared_ptr<InvariantNode>>& VarNode::staticInputTo() const noexcept {
  return _staticInputTo;
}

const std::vector<std::shared_ptr<InvariantNode>>& VarNode::dynamicInputTo() const noexcept {
  return _dynamicInputTo;
}

const std::unordered_set<std::shared_ptr<InvariantNode>>&
VarNode::definingNodes() const noexcept {
  return _outputOf;
}

std::shared_ptr<InvariantNode> VarNode::outputOf() const {
  if (_outputOf.empty()) {
    return nullptr;
  }
  if (_outputOf.size() != 1) {
    throw std::runtime_error("VarNode is not an output var");
  }
  return *_outputOf.begin();
}

void VarNode::markAsInputFor(InvariantNode& listeningInvariant,
                             const bool isStaticInput) {
  if (isStaticInput) {
    _staticInputTo.emplace_back(listeningInvariant.ptr());
  } else {
    _dynamicInputTo.emplace_back(listeningInvariant.ptr());
  }
}

void VarNode::unmarkOutputTo(const std::shared_ptr<InvariantNode>& definingInvNodeId) {
  _outputOf.erase(definingInvNodeId);
}

void VarNode::unmarkAsInputFor(const InvariantNode& listeningInvariant,
                               const bool isStaticInput) {
  if (isStaticInput) {
    for (Int i = static_cast<Int>(_staticInputTo.size()) - 1; i >= 0; --i) {
      if (_staticInputTo[i].get() == &listeningInvariant) {
        _staticInputTo.erase(_staticInputTo.begin() + i);
      }
    }
  } else {
    for (Int i = static_cast<Int>(_dynamicInputTo.size()) - 1; i >= 0; --i) {
      if (_dynamicInputTo[i].get() == &listeningInvariant) {
        _dynamicInputTo.erase(_dynamicInputTo.begin() + i);
      }
    }
  }
}

void VarNode::markOutputTo(InvariantNode& definingInvariant) {
  _outputOf.emplace(definingInvariant.ptr());
}

std::optional<Int> VarNode::constantValue() const noexcept {
  auto [lb, ub] = _domain->bounds();
  return lb == ub ? std::optional<Int>{lb} : std::optional<Int>{};
}

std::ostream& VarNode::dotLangIdentifier(std::ostream& o,
                                         const std::string& identifier) const {
  return o << reinterpret_cast<size_t>(this) << " [label=\"" << identifier << "\"];" << std::endl;
}

}  // namespace atlantis::invariantgraph
