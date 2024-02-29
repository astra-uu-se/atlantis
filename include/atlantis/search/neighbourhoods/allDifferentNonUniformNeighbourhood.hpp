#pragma once

#include <algorithm>
#include <cassert>

#include "atlantis/search/randomProvider.hpp"
#include "atlantis/search/searchVariable.hpp"
#include "neighbourhood.hpp"

namespace atlantis::search::neighbourhoods {

class AllDifferentNonUniformNeighbourhood : public Neighbourhood {
 private:
  std::vector<search::SearchVar> _vars;
  std::vector<size_t> _varIndices;
  const Int _domainOffset;
  // _valueIndexToVarIndex[i]:
  //  if _valueIndexToVarIndex[i] < _vars.size(), then
  //    vars[_valueIndexToVarIndex[i]] = _offset + i
  //  otherwise,
  //    no variable in _vars take value _offset + i
  std::vector<size_t> _valueIndexToVarIndex;

  // domains[i] = domain of _vars[i]
  std::vector<std::vector<Int>> _domains;
  // inDomain[i][j] = the domain of _vars[i] contains value j + _offset
  std::vector<std::vector<bool>> _inDomain;
  const propagation::SolverBase& _solver;

 public:
  AllDifferentNonUniformNeighbourhood(std::vector<search::SearchVar>&& vars,
                                      Int domainLb, Int domainUb,
                                      const propagation::SolverBase& solver);

  ~AllDifferentNonUniformNeighbourhood() override = default;

  void initialise(RandomProvider& random,
                  AssignmentModifier& modifications) override;
  bool randomMove(RandomProvider& random, Assignment& assignment,
                  Annealer& annealer) override;

  [[nodiscard]] const std::vector<SearchVar>& coveredVars() const override {
    return _vars;
  }
  [[nodiscard]] bool canSwap(const Assignment& assignment, size_t var1Index,
                             size_t val2Index) const noexcept;
  bool swapValues(Assignment& assignment, Annealer& annealer, size_t var1Index,
                  size_t val2Index);
  bool assignValue(Assignment& assignment, Annealer& annealer, size_t varIndex,
                   size_t newValIndex);

 private:
  [[nodiscard]] inline Int toValue(size_t valueIndex) const noexcept {
    assert(valueIndex < _valueIndexToVarIndex.size());
    assert(_valueIndexToVarIndex.at(valueIndex) <= _vars.size());
    return static_cast<Int>(valueIndex) + _domainOffset;
  }
  [[nodiscard]] inline size_t toValueIndex(Int value) const noexcept {
    assert(value >= _domainOffset);
    assert(static_cast<size_t>(value - _domainOffset) <
           _valueIndexToVarIndex.size());
    assert(_valueIndexToVarIndex.at(
               static_cast<size_t>(value - _domainOffset)) <= _vars.size());
    return static_cast<size_t>(value - _domainOffset);
  }
  [[nodiscard]] inline bool isValueIndexOccupied(
      size_t valueIndex) const noexcept {
    assert(valueIndex < _valueIndexToVarIndex.size());
    assert(_valueIndexToVarIndex.at(valueIndex) <= _vars.size());
    return _valueIndexToVarIndex[valueIndex] < _vars.size();
  }
  [[nodiscard]] inline bool inDomain(size_t varIndex,
                                     size_t valueIndex) const noexcept {
    assert(varIndex < _inDomain.size());
    assert(valueIndex < _inDomain.at(varIndex).size());
    return _inDomain[varIndex][valueIndex];
  }

#ifndef NDEBUG
  bool sanity(Assignment& assignment) {
    for (size_t varIndex = 0; varIndex < _vars.size(); ++varIndex) {
      const Int value = assignment.value(_vars.at(varIndex).solverId());
      const size_t valueIndex = toValueIndex(value);
      assert(valueIndex < _valueIndexToVarIndex.size());
      assert(_valueIndexToVarIndex.at(valueIndex) == varIndex);
    }
    return true;
  }
#endif
};

}  // namespace atlantis::search::neighbourhoods
