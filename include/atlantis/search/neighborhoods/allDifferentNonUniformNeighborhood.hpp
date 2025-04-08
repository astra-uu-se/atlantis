#pragma once

#include <array>
#include <cassert>

#include "atlantis/search/assignment.hpp"
#include "atlantis/search/neighborhoods/neighborhood.hpp"
#include "atlantis/search/searchVariable.hpp"

namespace atlantis::propagation {
class CommittableInt;
}

namespace atlantis::search {
class SearchVar;
}

namespace atlantis::search::neighborhoods {

class AllDifferentNonUniformNeighborhood : public Neighborhood {
  std::vector<SearchVar> _vars;
  std::vector<size_t> _varIndices;
  std::vector<propagation::CommittableInt> _domIndices;
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
  Timestamp _curTimestamp;
  std::array<size_t, 2> _moveValueIndex{};

 public:
  AllDifferentNonUniformNeighborhood(std::vector<SearchVar>&& vars,
                                     Int domainLb, Int domainUb);

  void initialize(RandomProvider&, Assignment&) override;

  size_t randomMove(RandomProvider&, Assignment&) override;

  void commitIf(const Assignment&) override;

  [[nodiscard]] const std::vector<SearchVar>& coveredVars() const override {
    return _vars;
  }
  [[nodiscard]] bool canSwap(const Assignment& assignment, size_t var1Index,
                             size_t value2Index) const noexcept;
  size_t swapValues(Assignment&, size_t var1Index, size_t value2Index);
  size_t assignValue(Assignment&, size_t varIndex, size_t newValueIndex);

 private:
  [[nodiscard]] Int toValue(size_t valueIndex) const noexcept {
    assert(valueIndex < _valueIndexToVarIndex.size());
    assert(_valueIndexToVarIndex.at(valueIndex) <= _vars.size());
    return static_cast<Int>(valueIndex) + _domainOffset;
  }
  [[nodiscard]] size_t toValueIndex(Int value) const noexcept {
    assert(value >= _domainOffset);
    assert(static_cast<size_t>(value - _domainOffset) <
           _valueIndexToVarIndex.size());
    assert(_valueIndexToVarIndex.at(
               static_cast<size_t>(value - _domainOffset)) <= _vars.size());
    return static_cast<size_t>(value - _domainOffset);
  }
  [[nodiscard]] bool isValueIndexOccupied(size_t valueIndex) const noexcept {
    assert(valueIndex < _valueIndexToVarIndex.size());
    assert(_valueIndexToVarIndex.at(valueIndex) <= _vars.size());
    return _valueIndexToVarIndex[valueIndex] < _vars.size();
  }
  [[nodiscard]] bool inDomain(size_t varIndex,
                              size_t valueIndex) const noexcept {
    assert(varIndex < _inDomain.size());
    assert(valueIndex < _inDomain.at(varIndex).size());
    return _inDomain[varIndex][valueIndex];
  }

#ifndef NDEBUG
  [[nodiscard]] bool sanity(const Assignment& assignment,
                            bool committedValue) const {
    for (size_t varIndex = 0; varIndex < _vars.size(); ++varIndex) {
      const Int value =
          committedValue
              ? assignment.committedValue(_vars.at(varIndex).solverId())
              : assignment.currentValue(_vars.at(varIndex).solverId());
      const size_t valueIndex = toValueIndex(value);
      assert(valueIndex < _valueIndexToVarIndex.size());
      assert(_valueIndexToVarIndex.at(valueIndex) == varIndex);
    }
    return true;
  }
#endif
};

}  // namespace atlantis::search::neighborhoods
