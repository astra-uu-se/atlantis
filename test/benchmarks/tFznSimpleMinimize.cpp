#include <string>

#include "modelTest.hpp"

namespace atlantis::testing {

TEST(FznSimpleMinimize, Solve) {
  const std::unordered_set<Int> validObjectives{1,2,3};
  testModelFile("test/simple_minimize.fzn", validObjectives);
}

}  // namespace atlantis::testing