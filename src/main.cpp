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
  a->commitValue(5);

  std::shared_ptr<Linear> abc =
      std::make_shared<Linear>(std::vector<Int>({1, 1}),
                               std::vector<std::shared_ptr<IntVar>>({a, b}), c);
  engine.registerInvariant(abc);
  abc->init(engine);
  std::cout << "end";
}