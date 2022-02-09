#include <chrono>
#include <iostream>
#include <random>

#include "constraints/allDifferent.hpp"
#include "core/propagationEngine.hpp"
#include "invariants/absDiff.hpp"
#include "invariants/linear.hpp"

void test();
int allIntervals(int);

int main() {
  static_assert("C++17");
  setLogLevel(info);

  const auto t1 = std::chrono::high_resolution_clock::now();
  int nProbes = allIntervals(25);
  const auto t2 = std::chrono::high_resolution_clock::now();

  auto duration =
      std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count();

  std::cout << "duration: " << duration << "ms\n"
            << "probes/ms: " << (((double)nProbes) / duration) << std::endl;
}

int allIntervals(int n) {
  PropagationEngine engine;
  engine.open();

  std::vector<VarId> s_vars;
  std::vector<VarId> v_vars;

  for (int i = 0; i < n; ++i) {
    s_vars.push_back(engine.makeIntVar(i, 0, n - 1));
  }

  for (int i = 1; i < n; ++i) {
    v_vars.push_back(engine.makeIntVar(i, 0, n - 1));
    engine.makeInvariant<AbsDiff>(s_vars[i - 1], s_vars[i], v_vars.back());
  }

  VarId violation = engine.makeIntVar(0, 0, n);
  engine.makeConstraint<AllDifferent>(violation, v_vars);

  engine.close();

  std::random_device rd;
  std::mt19937 gen = std::mt19937(rd());

  std::uniform_int_distribution<> distribution{0, n - 1};
  int nProbes = 0;
  for (int it = 0; it < 50000; ++it) {
    // Probe all swaps
    for (size_t i = 0; i < static_cast<size_t>(n); ++i) {
      for (size_t j = i + 1; j < static_cast<size_t>(n); ++j) {
        Int oldI = engine.getNewValue(s_vars[i]);
        Int oldJ = engine.getNewValue(s_vars[j]);
        engine.beginMove();
        engine.setValue(s_vars[i], oldJ);
        engine.setValue(s_vars[j], oldI);
        engine.endMove();

        engine.beginQuery();
        engine.query(violation);
        engine.endQuery();
        ++nProbes;
      }
    }
    const int i = distribution(gen);
    const int j = distribution(gen);
    const Int oldI = engine.getNewValue(s_vars[i]);
    const Int oldJ = engine.getNewValue(s_vars[j]);
    // Perform random swap
    engine.beginMove();
    engine.setValue(s_vars[i], oldJ);
    engine.setValue(s_vars[j], oldI);
    engine.endMove();

    engine.beginCommit();
    engine.query(violation);
    engine.endCommit();
    logDebug("Violation: " << engine.getCommittedValue(violation));
  }
  return nProbes;
}

void test() {
  PropagationEngine engine;
  engine.open();
  const VarId a = engine.makeIntVar(1, 1, 1);
  const VarId b = engine.makeIntVar(2, 2, 2);
  const VarId c = engine.makeIntVar(3, 3, 3);
  const VarId d = engine.makeIntVar(4, 4, 4);
  const VarId e = engine.makeIntVar(5, 5, 5);
  const VarId f = engine.makeIntVar(6, 6, 6);
  const VarId g = engine.makeIntVar(7, 7, 7);

  auto abc = engine.makeInvariant<Linear>(std::vector<Int>({1, 1}),
                                          std::vector<VarId>({a, b}), c);

  auto def = engine.makeInvariant<Linear>(std::vector<Int>({2, 2}),
                                          std::vector<VarId>({d, e}), f);
  auto acfg = engine.makeInvariant<Linear>(std::vector<Int>({3, 2, 1}),
                                           std::vector<VarId>({a, c, f}), g);

  engine.close();
  logInfo("----- end setup -----");

  std::random_device rd;
  std::mt19937 gen = std::mt19937(rd());

  std::uniform_int_distribution<> distribution{-100000, 100000};
  for (int i = 0; i < 1000000; ++i) {
    engine.beginMove();
    engine.setValue(a, distribution(gen));
    engine.setValue(c, distribution(gen));
    engine.endMove();

    engine.beginQuery();
    engine.query(g);
    engine.endQuery();
  }

  logInfo("----- done -----");
}
