#include "atlantis/invariantgraph/constraintSolver.hpp"

#include <ranges>
#include <vector>

#include "atlantis/invariantgraph/invariantGraph.hpp"
#include "atlantis/invariantgraph/varNode.hpp"
#include "atlantis/utils/domains.hpp"

using atlantis::propagation::SolverBase;

namespace atlantis::invariantgraph {

ConstraintSolver::ConstraintSolver() : _optVar(std::nullopt), _optVarIsInt(true) {}

ConstraintSolver::ConstraintSolver(ConstraintSolver& space) :
      Space(space),
      _optVar{space._optVar},
      _optVarIsInt{space._optVarIsInt} {
  _bv.resize(space._bv.size());
  for (size_t i = 0; i < space._bv.size(); ++i) {
    _bv[i].update(*this, space._bv[i]);
  }
  _iv.resize(space._iv.size());
  for (size_t i = 0; i < space._iv.size(); ++i) {
    _iv[i].update(*this, space._iv[i]);
  }
}

size_t ConstraintSolver::newIntVar(const Int value) {
  _iv.emplace_back(*this, value, value);
  return _iv.size();
}

size_t ConstraintSolver::newIntVar(const SearchDomain& dom) {
  if (dom.isInterval()) {
    _iv.emplace_back(*this, dom.lowerBound(), dom.upperBound());
  } else {
    std::vector<int> values(dom.size());
    size_t i = 0;
    for (auto iter = dom.begin(); iter != dom.end(); ++iter) {
      values[i++] = static_cast<int>(*iter);
    }
    _iv.emplace_back(*this, Gecode::IntSet(values.data(), static_cast<Int>(values.size())));
  }
  return _iv.size();
}

size_t ConstraintSolver::newBoolVar(const bool value) {
  _bv.emplace_back(*this, value ? 1 : 0, value ? 1 : 0);
  return _bv.size();
}

size_t ConstraintSolver::newBoolVar() {
  _bv.emplace_back(*this, 0, 1);
  return _bv.size();
}

void ConstraintSolver::setOptVar(const VarNode& var, const bool isIntVar) {
  _optVarIsInt = isIntVar;
  _optVar = var.constraintVarId();
}

Gecode::Space* ConstraintSolver::copy() {
  return new ConstraintSolver(*this);
}

}  // namespace atlantis::invariantgraph
