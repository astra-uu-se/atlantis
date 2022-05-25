#pragma once

#include "move.hpp"
#include "randomProvider.hpp"

namespace search {

/**
 * Annealing based on chapter 12 of:
 *
 * P. Van Hentenryck and L. Michel. Constraint-Based Local Search. The MIT
 * Press, 2005.
 */
class Annealer {
 private:
  const Assignment& _assignment;
  RandomProvider& _random;
  int _nb;
  float _temperature{2.0};
  int _iterations{0};
  float _cutoff{0.1};
  int _sf{3};
  float _mp{0.02};
  float _chest{90};
  float _factor{0.95};
  int _freezeCounter{0};
  int _ch{0};
  float _nc{_cutoff * static_cast<float>(_nb) * _chest};

 public:
  Annealer(const Assignment& assignment, RandomProvider& random)
      : _assignment(assignment),
        _random(random),
        _nb{static_cast<int>(assignment.searchVariables().size())} {}
  virtual ~Annealer() = default;

  [[nodiscard]] bool hasNextLocal() const { return _freezeCounter < 5; }

  void nextLocal() {
    _temperature = _factor * _temperature;
    if (1.0 * _ch / _temperature < _mp) {
      ++_freezeCounter;
    }

    _ch = 0;
    _iterations = 0;
  }

  bool exploreLocal() {
    ++_iterations;

    return static_cast<float>(_iterations) <=
               static_cast<float>(_sf) * _chest * static_cast<float>(_nb) &&
           static_cast<float>(_ch) < _nc;
  }

  /**
   * Determine whether @p move should be committed to the assignment.
   *
   * @tparam N The size of the move.
   * @param move The move itself.
   * @return True if @p move should be committed, false otherwise.
   */
  template <unsigned int N>
  bool acceptMove(Move<N> move) {
    // TODO: Weights for the objective and violation.
    Int assignmentCost = _assignment.cost().evaluate(1, 1);
    Int moveCost = move.probe(_assignment).evaluate(1, 1);
    Int delta = moveCost - assignmentCost;

    if (delta < 0) {
      applyImprove();
      return true;
    } else if (delta == 0) {
      return true;
    } else if (accept(-delta)) {
      applyAnnealingAction();
      return true;
    }

    return false;
  }

 private:
  void applyImprove() {
    ++_ch;
    if (_assignment.satisfiesConstraints()) {
      _freezeCounter = 0;
    }
  }

  void applyAnnealingAction() { ++_ch; }

 protected:
  [[nodiscard]] virtual bool accept(Int val) const {
    return std::exp(static_cast<float>(val) / _temperature) >=
           _random.floatInRange(0.0f, 1.0f);
  }
};

}  // namespace search