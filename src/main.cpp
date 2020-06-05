#include <core/engine.hpp>
#include <core/intVar.hpp>
#include <core/tracer.hpp>
#include <invariants/linear.hpp>
#include <iostream>
#include <utility>
#include <vector>
int main() {
  static_assert("C++17");
  std::cout << "hello world!" << std::endl;
  Engine engine;
  std::shared_ptr<IntVar> a = engine.makeIntVar();
  std::shared_ptr<IntVar> b = engine.makeIntVar();
  std::shared_ptr<IntVar> c = engine.makeIntVar();
  std::shared_ptr<IntVar> d = engine.makeIntVar();
  std::shared_ptr<IntVar> e = engine.makeIntVar();
  std::shared_ptr<IntVar> f = engine.makeIntVar();
  std::shared_ptr<IntVar> g = engine.makeIntVar();
  a->commitValue(1);
  b->commitValue(2);
  c->commitValue(3);
  d->commitValue(4);
  e->commitValue(5);
  f->commitValue(6);
  g->commitValue(7);

  std::shared_ptr<Linear> abc =
      std::make_shared<Linear>(std::vector<Int>({1, 1}),
                               std::vector<std::shared_ptr<IntVar>>({a, b}), c);
  engine.registerInvariant(abc);
  abc->init(engine);

  std::shared_ptr<Linear> def =
      std::make_shared<Linear>(std::vector<Int>({2, 2}),
                               std::vector<std::shared_ptr<IntVar>>({d, e}), f);
  engine.registerInvariant(def);
  def->init(engine);

  std::shared_ptr<Linear> acfg = std::make_shared<Linear>(
      std::vector<Int>({3, 2, 1}),
      std::vector<std::shared_ptr<IntVar>>({a, c, f}), g);
  engine.registerInvariant(acfg);
  acfg->init(engine);
  std::cout << "----- end setup -----\n";

  std::cout << "----- timestamp 1 -----\n";
  Timestamp timestamp = 0;
  {
    timestamp = 1;
    a->setNewValue(timestamp, 2);
    abc->notifyIntChanged(timestamp, engine, 0, 1, 2, 1);
    std::cout << "new value of c: " << c->getValue(timestamp) << "\n";
    acfg->notifyIntChanged(timestamp, engine, 0, a->getCommittedValue(),
                           a->getValue(timestamp), 3);
    std::cout << "new value of g: " << g->getValue(timestamp) << "\n";
    acfg->notifyIntChanged(timestamp, engine, 1, c->getCommittedValue(),
                           c->getValue(timestamp), 2);
    std::cout << "new value of g: " << g->getValue(timestamp) << "\n";
  }
  std::cout << "----- timestamp 2 -----\n";
  {
    timestamp = 2;
    a->setNewValue(timestamp, 0);
    abc->notifyIntChanged(timestamp, engine, 0, a->getCommittedValue(),
                          a->getValue(timestamp), 1);
    std::cout << "new value of c: " << c->getValue(timestamp) << "\n";
    acfg->notifyIntChanged(timestamp, engine, 0, a->getCommittedValue(),
                           a->getValue(timestamp), 3);
    std::cout << "new value of g: " << g->getValue(timestamp) << "\n";
    acfg->notifyIntChanged(timestamp, engine, 1, c->getCommittedValue(),
                           c->getValue(timestamp), 2);
    std::cout << "new value of g: " << g->getValue(timestamp) << "\n";
  }
}