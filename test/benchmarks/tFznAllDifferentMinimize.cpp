#include <string>

#include "modelTest.hpp"

namespace atlantis::testing {

TEST(FznAllDifferentMinimize, Solve) {
  testModelFile("all_different_minimize.fzn");
}

}  // namespace atlantis::testing