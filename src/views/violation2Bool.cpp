#include "views/violation2Bool.hpp"

#include "core/engine.hpp"

static Int convert(Int value) { return std::min<Int>(value, 1); }

Int Violation2Bool::value(Timestamp ts) const {
  return convert(_engine->value(ts, _parentId));
}

Int Violation2Bool::committedValue() const {
  return convert(_engine->committedValue(_parentId));
}
