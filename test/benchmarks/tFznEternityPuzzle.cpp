#include <string>

#include "modelTest.hpp"

namespace atlantis::testing {

TEST(FznEternityPuzzle, Solve) {
  testModelFile("test/eternity_16x16_actual.fzn");
}

}  // namespace atlantis::testing