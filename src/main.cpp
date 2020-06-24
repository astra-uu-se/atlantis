#include <iostream>
#include <random>
#include <utility>
#include <vector>

#include "core/engine.hpp"
#include "core/intVar.hpp"
#include "core/tracer.hpp"
#include "invariants/linear.hpp"
int main() {
  static_assert("C++17");
  std::cout << "hello world!" << std::endl;
  Engine engine;
  engine.open();
  VarId a = engine.makeIntVar(1);
  VarId b = engine.makeIntVar(2);
  VarId c = engine.makeIntVar(3);
  VarId d = engine.makeIntVar(4);
  VarId e = engine.makeIntVar(5);
  VarId f = engine.makeIntVar(6);
  VarId g = engine.makeIntVar(7);

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
  for (size_t i = 0; i < 1000000; i++) {
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