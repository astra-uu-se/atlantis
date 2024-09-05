#include <cassert>
#include <iostream>
#include <random>
#include <string>
#include <utility>
#include <vector>

#include "atlantis/propagation/invariants/linear.hpp"
#include "atlantis/propagation/solver.hpp"
#include "atlantis/propagation/views/intOffsetView.hpp"
#include "atlantis/propagation/violationInvariants/allDifferent.hpp"

using namespace atlantis::propagation;

inline bool all_in_range(size_t minInclusive, size_t maxExclusive,
                         std::function<bool(size_t)>&& predicate) {
  std::vector<size_t> vec(maxExclusive - minInclusive);
  std::iota(vec.begin(), vec.end(), minInclusive);
  return std::all_of(vec.begin(), vec.end(), std::move(predicate));
}

class Queens {
 public:
  std::shared_ptr<Solver> solver;
  std::vector<VarViewId> queens;
  std::vector<VarViewId> q_offset_plus;
  std::vector<VarViewId> q_offset_minus;
  std::random_device rd;
  std::mt19937 gen;

  std::uniform_int_distribution<int> distribution;
  int n;

  VarViewId violation1 = NULL_ID;
  VarViewId violation2 = NULL_ID;
  VarViewId violation3 = NULL_ID;
  VarViewId totalViolation = NULL_ID;

  Queens(int n_, PropagationMode propMode = PropagationMode::INPUT_TO_OUTPUT,
         OutputToInputMarkingMode markingMode = OutputToInputMarkingMode::NONE)
      : solver(std::make_shared<Solver>()), n(n_) {
    if (n < 0) {
      throw std::runtime_error("n must be non-negative.");
    }

    solver->open();
    solver->setPropagationMode(propMode);
    solver->setOutputToInputMarkingMode(markingMode);

    // the total number of variables is linear in n
    queens = std::vector<VarViewId>(n);
    q_offset_minus = std::vector<VarViewId>(n);
    q_offset_plus = std::vector<VarViewId>(n);

    for (int i = 0; i < n; ++i) {
      queens.at(i) = solver->makeIntVar(i, 0, n - 1);
      q_offset_minus.at(i) =
          solver->makeIntView<IntOffsetView>(*solver, queens.at(i), -i);
      q_offset_plus.at(i) =
          solver->makeIntView<IntOffsetView>(*solver, queens.at(i), i);
    }

    violation1 = solver->makeIntVar(0, 0, n);
    violation2 = solver->makeIntVar(0, 0, n);
    violation3 = solver->makeIntVar(0, 0, n);

    // 3 invariants, each having taking n static input variables
    solver->makeViolationInvariant<AllDifferent>(
        *solver, violation1, std::vector<VarViewId>(queens));
    solver->makeViolationInvariant<AllDifferent>(
        *solver, violation2, std::vector<VarViewId>(q_offset_minus));
    solver->makeViolationInvariant<AllDifferent>(
        *solver, violation3, std::vector<VarViewId>(q_offset_plus));

    totalViolation = solver->makeIntVar(0, 0, 3 * n);

    solver->makeInvariant<Linear>(
        *solver, totalViolation,
        std::vector<VarViewId>{violation1, violation2, violation3});

    solver->close();

    gen = std::mt19937(rd());

    distribution = std::uniform_int_distribution<int>{0, n - 1};
  }

  void solve(int maxIt) {
    int it = 0;

    std::vector<int> tabu;
    tabu.assign(n, 0);
    const int tenure = 10;
    bool done = false;

    while (it < maxIt && !done) {
      size_t bestI = 0;
      size_t bestJ = 0;
      int bestViol = n;
      for (size_t i = 0; i < static_cast<size_t>(n); ++i) {
        for (size_t j = i + 1; j < static_cast<size_t>(n); ++j) {
          if (tabu[i] > it && tabu[j] > it) {
            continue;
          }
          const int oldI = solver->committedValue(queens[i]);
          const int oldJ = solver->committedValue(queens[j]);
          solver->beginMove();
          solver->setValue(queens[i], oldJ);
          solver->setValue(queens[j], oldI);
          solver->endMove();

          solver->beginProbe();
          solver->query(totalViolation);
          solver->endProbe();

          assert(sanity());

          int newValue = solver->currentValue(totalViolation);
          if (newValue <= bestViol) {
            bestViol = newValue;
            bestI = i;
            bestJ = j;
          }
        }
      }

      const int oldI = solver->committedValue(queens[bestI]);
      const int oldJ = solver->committedValue(queens[bestJ]);
      solver->beginMove();
      solver->setValue(queens[bestI], oldJ);
      solver->setValue(queens[bestJ], oldI);
      solver->endMove();

      solver->beginCommit();
      solver->query(totalViolation);
      solver->endCommit();

      tabu[bestI] = it + tenure;
      tabu[bestJ] = it + tenure;
      if (bestViol == 0) {
        done = true;
      }
      ++it;
    }
  }

  std::string instanceToString() {
    std::string str = "Queens: ";
    for (auto q : queens) {
      str += std::to_string(solver->committedValue(q)) + ", ";
    }
    return str;
  }

  inline bool sanity() const {
    return all_in_range(0, n - 1, [&](const size_t i) {
      return all_in_range(i + 1, n, [&](const size_t j) {
        return solver->committedValue(queens.at(i)) !=
                   solver->committedValue(queens.at(j)) &&
               solver->currentValue(queens.at(i)) !=
                   solver->currentValue(queens.at(j));
      });
    });
  }
};

int main(int argc, char* argv[]) {
  if (argc < 2) {
    std::cerr << "usage: queens [NUM]";
    return 1;
  }
  int num = std::stoi(argv[1]);
  Queens q(num);
  q.solve(100000);
  std::cout << q.instanceToString() << "\n";
}
