#include <iostream>
#include <random>
#include <utility>
#include <vector>

#include "core/engine.hpp"
#include "core/intVar.hpp"
#include "core/tracer.hpp"
#include "invariants/linear.hpp"

void test();
void allIntervals(int);
void magicSquare(int);

int main() {
  static_assert("C++17");
  std::cout << "hello world!" << std::endl;
  test();
}

void magicSquare(int n) {
  int n2 = n * n;
  Engine engine;
  engine.open();

  std::vector<std::vector<VarId>> square;
  std::vector<VarId> flat;

  for (int i = 0; i < n; i++) {
    square.push_back(std::vector<VarId>{});
    for (int j = 0; j < n; j++) {
      auto var = engine.makeIntVar(i*n+j+1, 1, n2);
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
  ones.assign(violations.size(),1);
  engine.makeInvariant<Linear>(ones, violations, totalViolation);
  engine.close();
}

void allIntervals(int) {
  Engine engine;
  engine.open();

  // std::vector<VarId> s_vars;
  // std::vector<VarId> v_vars;

  // for (int i = 0; i < n; i++) {
  //   s_vars.push_back(engine.makeIntVar(i, 0, n - 1));
  // }

  // for (int i = 1; i < n; i++) {
  //   v_vars.push_back(engine.makeIntVar(i, 0, n - 1));
  //   engine.makeInvariant<AbsDiff>(s_vars.at(i - 1), s_vars.at(i),
  //                                 v_vars.back());
  // }

  // VarId violation = engine.makeIntVar(0,0,n);
  // engine.makeConstraint<AllDifferent>(violation, v_vars);

  engine.close();
}

void test() {
  Engine engine;
  engine.open();
  VarId a = engine.makeIntVar(1, 1, 1);
  VarId b = engine.makeIntVar(2, 2, 2);
  VarId c = engine.makeIntVar(3, 3, 3);
  VarId d = engine.makeIntVar(4, 4, 4);
  VarId e = engine.makeIntVar(5, 5, 5);
  VarId f = engine.makeIntVar(6, 6, 6);
  VarId g = engine.makeIntVar(7, 7, 7);

  auto abc = engine.makeInvariant<Linear>(std::vector<Int>({1, 1}),
                                          std::vector<VarId>({a, b}), c);

  auto def = engine.makeInvariant<Linear>(std::vector<Int>({2, 2}),
                                          std::vector<VarId>({d, e}), f);
  auto acfg = engine.makeInvariant<Linear>(std::vector<Int>({3, 2, 1}),
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