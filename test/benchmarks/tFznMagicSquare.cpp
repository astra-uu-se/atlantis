#include <string>

#include "modelTest.hpp"

namespace atlantis::testing {

TEST(FznMagicSquare, Solve) {
  const std::vector<std::vector<Int>> expectedSolutions{
      {2, 9, 4, 7, 5, 3, 6, 1, 8}, {2, 7, 6, 9, 5, 1, 4, 3, 8},
      {4, 9, 2, 3, 5, 7, 8, 1, 6}, {4, 3, 8, 9, 5, 1, 2, 7, 6},
      {6, 7, 2, 1, 5, 9, 8, 3, 4}, {6, 1, 8, 7, 5, 3, 2, 9, 4},
      {8, 3, 4, 1, 5, 9, 6, 7, 2}, {8, 1, 6, 3, 5, 7, 4, 9, 2}};
  testModelFile("test/magic_square.fzn");
}

}  // namespace atlantis::testing