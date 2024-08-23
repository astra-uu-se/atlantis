#include <string>

#include "modelTest.hpp"

namespace atlantis::testing {

TEST(FznAllDifferentMinimize, Solve) {
  testModelFile("all_different_minimize.fzn");
}

TEST(FznAllDifferentMinimize, Gecode) {
  testModelFile("all_different_minimize_gecode.fzn");
}

}  // namespace atlantis::testing