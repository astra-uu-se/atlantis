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
  VarId a = engine.makeIntVar();
  VarId b = engine.makeIntVar();
  VarId c = engine.makeIntVar();
  VarId d = engine.makeIntVar();
  VarId e = engine.makeIntVar();
  VarId f = engine.makeIntVar();
  VarId g = engine.makeIntVar();
  engine.setValue(a, 1);
  engine.setValue(b, 2);
  engine.setValue(c, 3);
  engine.setValue(d, 4);
  engine.setValue(e, 5);
  engine.setValue(f, 6);
  engine.setValue(g, 7);

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