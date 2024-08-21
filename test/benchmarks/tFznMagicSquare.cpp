#include <string>

#include "modelTest.hpp"

namespace atlantis::testing {

TEST(FznMagicSquare, Solve) { testModelFile("magic_square.fzn"); }

TEST(FznMagicSquare, Gecode) { testModelFile("magic_square_gecode.fzn"); }

}  // namespace atlantis::testing