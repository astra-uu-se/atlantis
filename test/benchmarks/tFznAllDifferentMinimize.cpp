#include <string>

#include "modelTest.hpp"

namespace atlantis::testing {

TEST(FznAllDifferentMinimize, Solve) {
  const std::unordered_set<Int> validObjectives{
      1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20};
  testModelFile("test/all_different_minimize.fzn", validObjectives);
}

}  // namespace atlantis::testing