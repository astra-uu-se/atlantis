#include <chrono>
#include <iostream>
#include <random>
#include <utility>
#include <vector>

#include "constraints/allDifferent.hpp"
#include "core/engine.hpp"
#include "core/intVar.hpp"
#include "core/propagationEngine.hpp"
#include "core/tracer.hpp"
#include "invariants/absDiff.hpp"
#include "invariants/linear.hpp"

void test();
int allIntervals(int);
void magicSquare(int);

int main() {
  static_assert("C++17");
  // test();

  auto t1 = std::chrono::high_resolution_clock::now();
  int nProbes = allIntervals(25);
  auto t2 = std::chrono::high_resolution_clock::now();

  auto duration =
      std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count();

  std::cout << "duration: " << duration << "ms\n"
            << "probes/ms: " << (((double)nProbes) / duration) << std::endl;
}

void magicSquare(int n) {
  int n2 = n * n;
  PropagationEngine engine;
  engine.open();

  std::vector<std::vector<VarId>> square;
  std::vector<VarId> flat;

  for (int i = 0; i < n; i++) {
    square.push_back(std::vector<VarId>{});
    for (int j = 0; j < n; j++) {
      auto var = engine.makeIntVar(i * n + j + 1, 1, n2);
      square.at(i).push_back(var);
      flat.push_back(var);
    }
  }

  std::vector<VarId> violations;

  for (int i = 0; i < n; i++) {
    /* code */
  }

  VarId totalViolation = engine.makeIntVar(0, 0, n2 * n2 * n2 * 1000);

  std::vector<Int> ones{};
  ones.assign(violations.size(), 1);
  engine.makeInvariant<Linear>(ones, violations, totalViolation);
  engine.close();
}

int allIntervals(int n) {
  PropagationEngine engine;
  engine.open();

  std::vector<VarId> s_vars;
  std::vector<VarId> v_vars;

  for (int i = 0; i < n; i++) {
    s_vars.push_back(engine.makeIntVar(i, 0, n - 1));
  }

  for (int i = 1; i < n; i++) {
    v_vars.push_back(engine.makeIntVar(i, 0, n - 1));
    engine.makeInvariant<AbsDiff>(s_vars.at(i - 1), s_vars.at(i),
                                  v_vars.back());
  }

  VarId violation = engine.makeIntVar(0, 0, n);
  engine.makeConstraint<AllDifferent>(violation, v_vars);

  engine.close();

  std::random_device rd;
  std::mt19937 gen = std::mt19937(rd());

  std::uniform_int_distribution<> distribution{0, n - 1};
  int nProbes = 0;
  for (int it = 0; it < 50000; it++) {
    // Probe all swaps
    for (size_t i = 0; i < static_cast<size_t>(n); i++) {
      for (size_t j = i + 1; j < static_cast<size_t>(n); j++) {
        Int oldI = engine.getValue(s_vars.at(i));
        Int oldJ = engine.getValue(s_vars.at(j));
        engine.beginMove();
        engine.setValue(s_vars.at(i), oldJ);
        engine.setValue(s_vars.at(j), oldI);
        engine.endMove();

        engine.beginQuery();
        engine.query(violation);
        engine.endQuery();
        ++nProbes;
      }
    }
    int i = distribution(gen);
    int j = distribution(gen);
    Int oldI = engine.getValue(s_vars.at(i));
    Int oldJ = engine.getValue(s_vars.at(j));
    // Perform random swap
    engine.beginMove();
    engine.setValue(s_vars.at(i), oldJ);
    engine.setValue(s_vars.at(j), oldI);
    engine.endMove();

    engine.beginCommit();
    engine.query(violation);
    engine.endCommit();
    // std::cout << "Violation: " << engine.getCommittedValue(violation) <<
    // "\n";
  }
  return nProbes;
}

void test() {
  PropagationEngine engine;
  engine.open();
  VarId a = engine.makeIntVar(1, 1, 1);
  VarId b = engine.makeIntVar(2, 2, 2);
  VarId c = engine.makeIntVar(3, 3, 3);
  VarId d = engine.makeIntVar(4, 4, 4);
  VarId e = engine.makeIntVar(5, 5, 5);
  VarId f = engine.makeIntVar(6, 6, 6);
  VarId g = engine.makeIntVar(7, 7, 7);

  engine.makeInvariant<Linear>(std::vector<Int>({1, 1}),
                                          std::vector<VarId>({a, b}), c);

  engine.makeInvariant<Linear>(std::vector<Int>({2, 2}),
                                          std::vector<VarId>({d, e}), f);
  engine.makeInvariant<Linear>(std::vector<Int>({3, 2, 1}),
                                           std::vector<VarId>({a, c, f}), g);

  engine.close();
  std::cout << "----- end setup -----\n";

  std::random_device rd;
  std::mt19937 gen = std::mt19937(rd());

  std::uniform_int_distribution<> distribution{-100000, 100000};
  for (int i = 0; i < 1000000; i++) {
    engine.beginMove();
    engine.setValue(a, distribution(gen));
    engine.setValue(c, distribution(gen));
    engine.endMove();

    engine.beginQuery();
    engine.query(g);
    engine.endQuery();
  }

  std::cout << "----- done -----\n";
}
