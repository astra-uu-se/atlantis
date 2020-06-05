#include <core/engine.hpp>
#include <core/intVar.hpp>
#include <invariants/linear.hpp>
#include <iostream>
#include <utility>
#include <vector>
int main() {
  static_assert("C++17");
  std::cout << "hello world!" << std::endl;
  Engine engine;
  std::shared_ptr<IntVar> a = std::make_shared<IntVar>();
  a->commitValue(5);
  std::shared_ptr<IntVar> b = std::make_shared<IntVar>();
  std::shared_ptr<IntVar> c = std::make_shared<IntVar>();
  std::shared_ptr<IntVar> d = std::make_shared<IntVar>();
  std::shared_ptr<IntVar> e = std::make_shared<IntVar>();
  std::shared_ptr<IntVar> f = std::make_shared<IntVar>();
  engine.registerIntVar({a, b, c, d, e, f});

  std::shared_ptr<Linear> abc =
      std::make_shared<Linear>(std::vector<Int>({1, 1}),
                               std::vector<std::shared_ptr<IntVar>>({a, b}), c);
  engine.registerInvariant(abc);
  abc->init(engine);
  std::cout << "end";
}