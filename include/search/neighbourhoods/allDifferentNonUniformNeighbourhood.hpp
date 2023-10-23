#pragma once

#include <cassert>

#include "neighbourhood.hpp"
#include "search/randomProvider.hpp"
#include "search/searchVariable.hpp"

namespace atlantis::search::neighbourhoods {

class AllDifferentNonUniformNeighbourhood : public Neighbourhood {
 private:
  std::vector<search::SearchVariable> _variables;
  std::vector<size_t> _variableIndices;
  const Int _domainOffset;
  // _valueIndexToVariableIndex[i]:
  //  if _valueIndexToVariableIndex[i] < _variables.size(), then
  //    variables[_valueIndexToVariableIndex[i]] = _offset + i
  //  otherwise,
  //    no variable in _variables take value _offset + i
  std::vector<size_t> _valueIndexToVariableIndex;

  // domains[i] = domain of _variables[i]
  std::vector<std::vector<Int>> _domains;
  // inDomain[i][j] = the domain of _variables[i] contains value j + _offset
  std::vector<std::vector<bool>> _inDomain;
  const propagation::SolverBase& _solver;

 public:
  AllDifferentNonUniformNeighbourhood(
      std::vector<search::SearchVariable>&& variables, Int domainLb,
      Int domainUb, const propagation::SolverBase& solver);

  ~AllDifferentNonUniformNeighbourhood() override = default;

  void initialise(RandomProvider& random,
                  AssignmentModifier& modifications) override;
  bool randomMove(RandomProvider& random, Assignment& assignment,
                  Annealer& annealer) override;

  const std::vector<SearchVariable>& coveredVariables() const override {
    return _variables;
  }
  bool canSwap(const Assignment& assignment, size_t var1Index,
               size_t val2Index) const noexcept;
  bool swapValues(Assignment& assignment, Annealer& annealer, size_t var1Index,
                  size_t val2Index);
  bool assignValue(Assignment& assignment, Annealer& annealer, size_t varIndex,
                   size_t newValIndex);

 private:
  inline Int toValue(size_t valueIndex) const noexcept {
    assert(valueIndex < _valueIndexToVariableIndex.size());
    assert(_valueIndexToVariableIndex.at(valueIndex) <= _variables.size());
    return static_cast<Int>(valueIndex) + _domainOffset;
  }
  inline size_t toValueIndex(Int value) const noexcept {
    assert(value >= _domainOffset);
    assert(static_cast<size_t>(value - _domainOffset) <
           _valueIndexToVariableIndex.size());
    assert(_valueIndexToVariableIndex.at(static_cast<size_t>(
               value - _domainOffset)) <= _variables.size());
    return static_cast<size_t>(value - _domainOffset);
  }
  inline bool isValueIndexOccupied(size_t valueIndex) const noexcept {
    assert(valueIndex < _valueIndexToVariableIndex.size());
    assert(_valueIndexToVariableIndex.at(valueIndex) <= _variables.size());
    return _valueIndexToVariableIndex[valueIndex] < _variables.size();
  }
  inline bool inDomain(size_t variableIndex, size_t valueIndex) const noexcept {
    assert(variableIndex < _inDomain.size());
    assert(valueIndex < _inDomain.at(variableIndex).size());
    return _inDomain[variableIndex][valueIndex];
  }

#ifndef NDEBUG
  bool sanity(Assignment& assignment) {
    for (size_t variableIndex = 0; variableIndex < _variables.size();
         ++variableIndex) {
      const Int value =
          assignment.value(_variables.at(variableIndex).solverId());
      const size_t valueIndex = toValueIndex(value);
      assert(valueIndex < _valueIndexToVariableIndex.size());
      assert(_valueIndexToVariableIndex.at(valueIndex) == variableIndex);
    }
    return true;
  }
#endif
};

}  // namespace atlantis::search::neighbourhoods
