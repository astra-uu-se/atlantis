#include "search/annealer.hpp"

#include <random>

bool search::Annealer::accept(Int currentObjectiveValue,
                              Int newObjectiveValue) const {
  static std::random_device rd;
  static std::mt19937 gen(rd());
  static std::uniform_real_distribution<> acceptanceDist(0.0, 1.0);

  auto delta = newObjectiveValue - currentObjectiveValue;
  if (delta < 0) return true;

  auto metropolisCriterion =
      exp(-static_cast<double>(newObjectiveValue - currentObjectiveValue) /
          _temperature);

  return acceptanceDist(gen) < metropolisCriterion;
}

bool search::Annealer::globalCondition() {
  static double CUTOFF = 0.1;
  static double COOLING_FACTOR = 0.95;

  _localIterations = 0;

  _temperature *= COOLING_FACTOR;
  return _temperature > CUTOFF;
}

bool search::Annealer::searchLocally() {
  static Int MAX_ITERATIONS = 10;

  ++_localIterations;
  return _localIterations <= MAX_ITERATIONS;
}
