#include <string>

#include "modelTest.hpp"

namespace atlantis::testing {

TEST(FznNQueens, Solve) { testModelFile("n_queens.fzn"); }

TEST(FznNQueens, Gecode) { testModelFile("n_queens_gecode.fzn"); }

}  // namespace atlantis::testing