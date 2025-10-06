#pragma once

#include "atlantis/types.hpp"

namespace atlantis::search {

class Assignment;
class Cost;
class RandomProvider;

class MetaHeuristic {
 public:
  virtual ~MetaHeuristic() = default;

  virtual void start() = 0;

  [[nodiscard]] virtual bool isFinished() const = 0;

  virtual bool acceptMove(const Cost& cost) = 0;
};

}  // namespace atlantis::search
