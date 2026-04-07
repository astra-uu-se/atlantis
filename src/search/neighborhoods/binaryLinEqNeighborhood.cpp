#include "atlantis/search/neighborhoods/binaryLinEqNeighborhood.hpp"

#include <algorithm>
#include <cassert>
#include <numeric>

#include "atlantis/search/assignment.hpp"
#include "atlantis/search/randomProvider.hpp"
#include "atlantis/utils/domains.hpp"
#include "helper.hpp"

namespace atlantis::search::neighborhoods {

constexpr Int MOVE_INCREMENT = 0;
constexpr Int MOVE_DECREMENT = 1;
constexpr Int MOVE_SWAP_POS = 2;
constexpr Int MOVE_SWAP_NEG = 3;

// If bound is negative, then multiply each coeff and the bound by -1
template <bool Violation>
BinaryLinEqNeighborhood<Violation>::BinaryLinEqNeighborhood(const std::vector<Int>& coeffs,
                          std::vector<SearchVar>&& vars, const Int bound)
    : _indices(coeffs.size()),
      _numNegative(std::ranges::count_if(coeffs, [bound](const Int& c) { return bound >= 0 ? c < 0 : c > 0; })),
      _numNegTrue{0},
      _numPosTrue{0},
      _bound(static_cast<size_t>(std::abs(bound))),
      _curTimestamp(NULL_TIMESTAMP),
      _index1(coeffs.size()),
      _index2(coeffs.size()),
      _moveType(std::numeric_limits<Int>::max()) {
  std::ranges::iota(_indices, 0);
  std::ranges::sort(_indices, [bound, &coeffs](const size_t i1, const size_t i2) {
    return bound >= 0 ? coeffs[i1] < coeffs[i2] : coeffs[i1] > coeffs[i2];
  });
  _vars.reserve(coeffs.size());
  for (size_t i = 0; i < vars.size(); ++i) {
    _vars.emplace_back(vars[_indices[i]]);
  }
}


template <bool Violation>
size_t BinaryLinEqNeighborhood<Violation>::numPositive() const noexcept {
  assert(_vars.size() >= _numNegative);
  return _vars.size() - _numNegative;
}

template <bool Violation>
bool BinaryLinEqNeighborhood<Violation>::canIncrement() const noexcept {
  return _numNegTrue < _numNegative && _numPosTrue < numPositive();
}

template <bool Violation>
bool BinaryLinEqNeighborhood<Violation>::canDecrement() const noexcept {
  return _numNegTrue > 0 && _numPosTrue > _bound;
}

template <bool Violation>
bool BinaryLinEqNeighborhood<Violation>::canSwapNeg() const noexcept {
  return 0 < _numNegTrue && _numNegTrue < _numNegative;
}

template <bool Violation>
bool BinaryLinEqNeighborhood<Violation>::canSwapPos() const noexcept {
  return 0 < _numPosTrue && _numPosTrue < numPositive();
}

template <bool Violation>
void BinaryLinEqNeighborhood<Violation>::initialize(RandomProvider& random,
                                   Assignment& assignment) {
  _moveType = std::numeric_limits<Int>::max();
  _curTimestamp = NULL_TIMESTAMP;

  std::ranges::iota(_indices, 0);
  for (size_t i = 0; i < _numNegative; ++i) {
    const Int idx = random.intInRange(static_cast<Int>(i), static_cast<Int>(_numNegative) - 1);
    std::swap(_indices[i], _indices[idx]);
  }
  for (size_t i = _numNegative; i < _vars.size(); ++i) {
    const Int idx = random.intInRange(static_cast<Int>(i), static_cast<Int>(_vars.size()) - 1);
    std::swap(_indices[i], _indices[idx]);
  }
  if (numPositive() <= _bound) {
    _numNegTrue = 0;
  } else {
    _numNegTrue = random.intInRange(0, std::min(_numNegative, numPositive() - _bound));
  }
  for (size_t i = 0; i < _numNegative; ++i) {
    assignment.set(_vars[_indices[i]].solverId(), toInt<Violation>(i < _numNegTrue));
  }
  _numPosTrue = _bound + _numNegTrue;
  for (size_t i = 0; i < numPositive(); ++i) {
    assignment.set(_vars[_indices[_numNegative + i]].solverId(), toInt<Violation>(i < _numPosTrue));
  }
}

template <bool Violation>
size_t BinaryLinEqNeighborhood<Violation>::randomMove(RandomProvider& random,
                                     Assignment& assignment) {
  std::vector<Int> availableMoves;
  availableMoves.reserve(4);
  if (canIncrement()) {
    availableMoves.emplace_back(MOVE_INCREMENT);
  }
  if (canDecrement()) {
    availableMoves.emplace_back(MOVE_DECREMENT);
  }
  if (canSwapNeg()) {
    availableMoves.emplace_back(MOVE_SWAP_NEG);
  }
  if (canSwapPos()) {
    availableMoves.emplace_back(MOVE_SWAP_POS);
  }
  assert(!availableMoves.empty());
  const auto move = random.element(availableMoves);
  switch (move) {
    case MOVE_INCREMENT:
      return incrementMove(random, assignment);
    case MOVE_DECREMENT:
      return decrementMove(random, assignment);
    case MOVE_SWAP_NEG:
      return swapNegMove(random, assignment);
    case MOVE_SWAP_POS:
      return swapPosMove(random, assignment);
    default:
      return 0;
  }
}

template <bool Violation>
size_t BinaryLinEqNeighborhood<Violation>::incrementMove(RandomProvider& random, Assignment& assignment) {
  _moveType = MOVE_INCREMENT;
  _curTimestamp = assignment.currentTimestamp();
  assert(canIncrement());
  assert(_numNegTrue < _numNegative);
  _index1 = random.intInRange(_numNegTrue, _numNegative - 1);
  assert(_numPosTrue < numPositive());
  assert(_numNegative + _numPosTrue < _vars.size());
  _index2 = random.intInRange(_numNegative + _numPosTrue, _vars.size() - 1);
  assert(!toBool<Violation>(assignment.committedValue(_vars.at(_indices.at(_index1)).solverId())));
  assignment.set(_vars[_indices[_index1]].solverId(), toInt<Violation>(true));
  assert(!toBool<Violation>(assignment.committedValue(_vars.at(_indices.at(_index2)).solverId())));
  assignment.set(_vars[_indices[_index2]].solverId(), toInt<Violation>(true));
  return 2;
}

template <bool Violation>
size_t BinaryLinEqNeighborhood<Violation>::decrementMove(RandomProvider& random, Assignment& assignment) {
  _moveType = MOVE_DECREMENT;
  _curTimestamp = assignment.currentTimestamp();
  assert(canDecrement());
  assert(_numNegTrue > 0);
  _index1 = random.intInRange(0, _numNegTrue - 1);
  assert(_numPosTrue > 0);
  assert(_numPosTrue > _bound);
  _index2 = random.intInRange(_numNegative, _numNegative + _numPosTrue - 1);
  assert(toBool<Violation>(assignment.committedValue(_vars.at(_indices.at(_index1)).solverId())));
  assignment.set(_vars[_indices[_index1]].solverId(), toInt<Violation>(false));
  assert(toBool<Violation>(assignment.committedValue(_vars.at(_indices.at(_index2)).solverId())));
  assignment.set(_vars[_indices[_index2]].solverId(), toInt<Violation>(false));
  return 2;
}

template <bool Violation>
size_t BinaryLinEqNeighborhood<Violation>::swapNegMove(RandomProvider& random, Assignment& assignment) {
  _moveType = MOVE_SWAP_NEG;
  _curTimestamp = assignment.currentTimestamp();
  assert(canSwapNeg());
  assert(0 < _numNegTrue);
  assert(_numNegTrue < _numNegative);
  _index1 = random.intInRange(0, _numNegTrue - 1);
  _index2 = random.intInRange(_numNegTrue, _numNegative - 1);
  assert(toBool<Violation>(assignment.committedValue(_vars.at(_indices.at(_index1)).solverId())));
  assignment.set(_vars[_indices[_index1]].solverId(), toInt<Violation>(false));
  assert(!toBool<Violation>(assignment.committedValue(_vars.at(_indices.at(_index2)).solverId())));
  assignment.set(_vars[_indices[_index2]].solverId(), toInt<Violation>(true));
  return 2;
}

template <bool Violation>
size_t BinaryLinEqNeighborhood<Violation>::swapPosMove(RandomProvider& random, Assignment& assignment) {
  _moveType = MOVE_SWAP_POS;
  _curTimestamp = assignment.currentTimestamp();
  assert(canSwapPos());
  assert(0 < _numPosTrue);
  assert(_numPosTrue < numPositive());
  _index1 = random.intInRange(_numNegative, _numNegative + _numPosTrue - 1);
  _index2 = random.intInRange(_numNegative + _numPosTrue, _vars.size() - 1);
  assert(toBool<Violation>(assignment.committedValue(_vars.at(_indices.at(_index1)).solverId())));
  assignment.set(_vars[_indices[_index1]].solverId(), toInt<Violation>(false));
  assert(!toBool<Violation>(assignment.committedValue(_vars.at(_indices.at(_index2)).solverId())));
  assignment.set(_vars[_indices[_index2]].solverId(), toInt<Violation>(true));
  return 2;
}


template <bool Violation>
void BinaryLinEqNeighborhood<Violation>::commitIf(const Assignment& assignment) {
  if (assignment.currentTimestamp() != _curTimestamp) {
    return;
  }
  const Int m = _moveType;
  _moveType = std::numeric_limits<Int>::max();
  if (m == MOVE_INCREMENT) {
    assert(_numNegTrue < _numNegative);
    assert(_index1 < _numNegative);
    std::swap(_indices[_numNegTrue], _indices[_index1]);
    assert(_numPosTrue < numPositive());
    assert(_numNegative <= _index2);
    assert(_index2 < _vars.size());
    std::swap(_indices[_numNegative + _numPosTrue], _indices[_index2]);
    ++_numNegTrue;
    ++_numPosTrue;
    return;
  }
  if (m == MOVE_DECREMENT) {
    assert(0 < _numNegTrue);
    assert(_index1 < _numNegTrue);
    assert(_bound < _numPosTrue);
    assert(_numNegative <= _index2);
    assert(_index2 < _numNegative + _numPosTrue);
    --_numNegTrue;
    --_numPosTrue;
    std::swap(_indices[_numNegTrue], _indices[_index1]);
    std::swap(_indices[_numNegative + _numPosTrue], _indices[_index2]);
    return;
  }
  if (m == MOVE_SWAP_NEG || m == MOVE_SWAP_POS) {
    assert(_index1 < _numNegTrue || (_numNegative <= _index1 && _index1 < _numNegative + _numPosTrue));
    assert((_numNegTrue <= _index2  && _index2 < _numNegative) ||
           (_numNegative + _numPosTrue <= _index2 && _index1 < _vars.size()));
    std::swap(_indices[_index1], _indices[_index2]);
  }
}

}  // namespace atlantis::search::neighborhoods
