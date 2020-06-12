#include <iostream>
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

  std::cout << "----- timestamp 1 -----\n";
  Timestamp timestamp = 0;
  {
    timestamp = 1;
    engine.setValue(timestamp, a, 2);
    abc->notifyIntChanged(timestamp, engine, 0, 1, 2, 1);
    std::cout << "new value of c: " << engine.getValue(timestamp, c) << "\n";
    acfg->notifyIntChanged(timestamp, engine, 0, engine.getCommitedValue(a),
                           engine.getValue(timestamp, a), 3);
    std::cout << "new value of g: " << engine.getValue(timestamp, g) << "\n";
    acfg->notifyIntChanged(timestamp, engine, 1, engine.getCommitedValue(c),
                           engine.getValue(timestamp, c), 2);
    std::cout << "new value of g: " << engine.getValue(timestamp, g) << "\n";
  }
  std::cout << "----- timestamp 2 -----\n";
  {
    timestamp = 2;
    engine.setValue(timestamp, a, 0);
    abc->notifyIntChanged(timestamp, engine, 0, engine.getCommitedValue(a),
                          engine.getValue(timestamp, a), 1);
    std::cout << "new value of c: " << engine.getValue(timestamp, c) << "\n";
    acfg->notifyIntChanged(timestamp, engine, 0, engine.getCommitedValue(a),
                           engine.getValue(timestamp, a), 3);
    std::cout << "new value of g: " << engine.getValue(timestamp, g) << "\n";
    acfg->notifyIntChanged(timestamp, engine, 1, engine.getCommitedValue(c),
                           engine.getValue(timestamp, c), 2);
    std::cout << "new value of g: " << engine.getValue(timestamp, g) << "\n";
  }
}