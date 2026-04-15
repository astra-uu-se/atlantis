#include "atlantis/invariantgraph/constraintSolver.hpp"

#include <boost/iostreams/categories.hpp>
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

Gecode::IntVar ConstraintSolver::intVar(const ConstraintVarId varId) {
  assert(varId < _iv.size());
  return _iv[varId];
}

Gecode::BoolVar ConstraintSolver::boolVar(const ConstraintVarId varId) {
  assert(varId < _bv.size());
  return _bv[varId];
}

Gecode::IntVarArgs ConstraintSolver::intVarArgs(const std::vector<ConstraintVarId>& varIds) {
  Gecode::IntVarArgs args(static_cast<int>(varIds.size()));
  for (int i = 0; i < static_cast<int>(varIds.size()); ++i) {
    args[i] = intVar(varIds[i]);
  }
  return args;
}

Gecode::BoolVarArgs ConstraintSolver::boolVarArgs(const std::vector<ConstraintVarId>& varIds) {
  Gecode::BoolVarArgs args(static_cast<int>(varIds.size()));
  for (int i = 0; i < static_cast<int>(varIds.size()); ++i) {
    args[i] = boolVar(varIds[i]);
  }
  return args;
}

size_t ConstraintSolver::newIntVar(const Int value) {
  const size_t ret = _iv.size();
  _iv.emplace_back(*this, value, value);
  return ret;
}

size_t ConstraintSolver::newIntVar(const SearchDomain& dom) {
  const size_t ret = _iv.size();
  if (dom.isInterval()) {
    _iv.emplace_back(*this, dom.lowerBound(), dom.upperBound());
  } else {
    std::vector<int> values(dom.size());
    size_t i = 0;
    for (auto iter = dom.begin(); iter != dom.end(); ++iter) {
      values[i++] = static_cast<int>(*iter);
    }
    _iv.emplace_back(*this, Gecode::IntSet(values.data(), static_cast<int>(values.size())));
  }
  return ret;
}

size_t ConstraintSolver::newBoolVar(const bool value) {
  const size_t ret = _bv.size();
  _bv.emplace_back(*this, value ? 1 : 0, value ? 1 : 0);
  return ret;
}

size_t ConstraintSolver::newBoolVar() {
  const size_t ret = _bv.size();
  _bv.emplace_back(*this, 0, 1);
  return ret;
}

void ConstraintSolver::setOptVar(const VarNode& var, const bool isIntVar) {
  _optVarIsInt = isIntVar;
  _optVar = var.constraintVarId();
}

void ConstraintSolver::fixPoint() {
  Gecode::SpaceStatus s = status();
  if (s == Gecode::SS_FAILED) {
    throw InconsistencyException("UNSAT");
  }
  assert(s == Gecode::SS_SOLVED);
}

SearchDomain ConstraintSolver::intVarDomain(const ConstraintVarId varId) const {
  if (_iv[varId].range()) {
    return SearchDomain(_iv[varId].min(), _iv[varId].max());
  }
  std::vector<Int> values(_iv[varId].size());
  size_t i = 0;
  for (int v = _iv[varId].min(); v <= _iv[varId].max(); ++v) {
    if (_iv[varId].in(v)) {
      values[i++] = v;
    }
  }
  return SearchDomain(std::move(values));
}

SearchDomain ConstraintSolver::boolVarDomain(const ConstraintVarId varId) const {
  if (_bv[varId].assigned()) {
    const Int viol = _bv[varId].val() == 0 ? 1 : 0;
    return SearchDomain(viol, viol);
  }
  return SearchDomain(0, 1);
}

Gecode::Space* ConstraintSolver::copy() {
  return new ConstraintSolver(*this);
}

void ConstraintSolver::array_bool_rel(const std::vector<ConstraintVarId>& inputs,
                                      const ConstraintVarId reified,
                                      const Gecode::BoolOpType op) {
  rel(*this, op, boolVarArgs(inputs), boolVar(reified), Gecode::IPL_BND);
}

void ConstraintSolver::array_bool_rel(const std::vector<ConstraintVarId>& inputs,
                                      const bool shouldHold,
                                      const Gecode::BoolOpType op) {
  rel(*this, op, boolVarArgs(inputs), shouldHold ? 1 : 0, Gecode::IPL_BND);
}

void ConstraintSolver::array_bool_and(const std::vector<ConstraintVarId>& inputs,
                                      const ConstraintVarId reified) {
  array_bool_rel(inputs, reified, Gecode::BOT_AND);
}

void ConstraintSolver::array_bool_and(const std::vector<ConstraintVarId>& inputs,
                                      const bool shouldHold) {
  array_bool_rel(inputs, shouldHold, Gecode::BOT_AND);
}

void ConstraintSolver::array_bool_element(const ConstraintVarId& index,
  const std::vector<Int>& parameters,
  const ConstraintVarId output,
  const Int offset) {
  Gecode::IntSharedArray sia(static_cast<int>(parameters.size()));
  for (int i = 0; i < static_cast<int>(parameters.size()); ++i) {
    sia[i] = static_cast<int>(parameters[i]);
  }
  element(*this, sia, intVar(index), -static_cast<int>(offset), boolVar(output));
}

void ConstraintSolver::array_bool_element2d(const ConstraintVarId& rowIndex, const ConstraintVarId& colIndex,
  const std::vector<std::vector<Int>>& parameters,
  const ConstraintVarId output,
  const Int rowOffset,
  const Int colOffset) {
  Gecode::IntSharedArray sia(static_cast<int>(parameters.size() * parameters.front().size()));
  int i = 0;
  for (const auto& row : parameters) {
    for (const Int val : row) {
      sia[i++] = static_cast<int>(val);
    }
  }
  element(*this, sia, intVar(colIndex), -static_cast<int>(colOffset), static_cast<int>(parameters.front().size()),
    intVar(rowIndex), -static_cast<int>(rowOffset), static_cast<int>(parameters.size()),
    boolVar(output), Gecode::IPL_DOM);
}

void ConstraintSolver::array_bool_or(const std::vector<ConstraintVarId>& inputs,
                                      const ConstraintVarId reified) {
  array_bool_rel(inputs, reified, Gecode::BOT_OR);
}

void ConstraintSolver::array_bool_or(const std::vector<ConstraintVarId>& inputs,
                                      const bool shouldHold) {
  array_bool_rel(inputs, shouldHold, Gecode::BOT_OR);
}

void ConstraintSolver::array_bool_xor(const std::vector<ConstraintVarId>& inputs,
                                      const ConstraintVarId reified) {
  array_bool_rel(inputs, reified, Gecode::BOT_XOR);
}

void ConstraintSolver::array_bool_xor(const std::vector<ConstraintVarId>& inputs,
                                      const bool shouldHold) {
  array_bool_rel(inputs, shouldHold, Gecode::BOT_XOR);
}

void ConstraintSolver::bool2int(const ConstraintVarId boolVarId, const ConstraintVarId intVarId) {
  channel(*this, boolVar(boolVarId), intVar(intVarId));
}


}  // namespace atlantis::invariantgraph
