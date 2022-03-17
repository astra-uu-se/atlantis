#include "search/move.hpp"

const search::Cost& search::Move::probe(const Assignment& assignment) {
  if (_probed) {
    _cost = assignment.probe([&] (auto& modifications) {
      modify(modifications);
    });

    _probed = true;
  }

  return _cost;
}

void search::Move::commit(search::Assignment& assignment) {
  assignment.assign([&] (auto& modifications) {
    modify(modifications);
  });
}

void search::AssignMove::modify(search::AssignmentModification& modifications) {
  modifications.set(_target, _value);
}
