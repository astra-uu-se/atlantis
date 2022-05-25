#include "utils/priorityList.hpp"

PriorityList::PriorityList(size_t size)
    : _minimum(NULL_TIMESTAMP, 0), _maximum(NULL_TIMESTAMP, 0) {
  _list.reserve(size);
  for (size_t i = 0; i < size; i++) {
    _list.emplace_back(NULL_TIMESTAMP, 0);
  }
}

size_t PriorityList::size() const noexcept { return _list.size(); }

Int PriorityList::minPriority(Timestamp ts) const noexcept {
  return _list[_minimum.get(ts)].get(ts);
}

Int PriorityList::maxPriority(Timestamp ts) const noexcept {
  return _list[_maximum.get(ts)].get(ts);
}

void PriorityList::commitIf(Timestamp ts) {
  for (auto& node : _list) {
    node.commitIf(ts);
  }

  _minimum.commitIf(ts);
  _maximum.commitIf(ts);
}

void PriorityList::updatePriority(Timestamp ts, size_t idx, Int newValue) {
  auto oldValue = _list[idx].get(ts);
  if (oldValue == newValue) {
    return;
  }

  auto min = minPriority(ts);
  auto max = maxPriority(ts);
  _list[idx].set(ts, newValue);

  if (newValue > max) {
    _maximum.set(ts, idx);
  } else if (oldValue == max && newValue < max) {
    computeMaximum(ts);
  }

  if (newValue < min) {
    _minimum.set(ts, idx);
  } else if (oldValue == min && newValue > min) {
    computeMinimum(ts);
  }
}

void PriorityList::computeMaximum(Timestamp ts) {
  size_t maximumIdx = 0;
  for (size_t idx = 1; idx < size(); idx++) {
    if (_list[idx].get(ts) > _list[maximumIdx].get(ts)) {
      maximumIdx = idx;
    }
  }

  _maximum.set(ts, maximumIdx);
}

void PriorityList::computeMinimum(Timestamp ts) {
  size_t minimumIdx = 0;
  for (size_t idx = 1; idx < size(); idx++) {
    if (_list[idx].get(ts) < _list[minimumIdx].get(ts)) {
      minimumIdx = idx;
    }
  }

  _minimum.set(ts, minimumIdx);
}
