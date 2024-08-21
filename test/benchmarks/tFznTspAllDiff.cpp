#include <string>

#include "modelTest.hpp"

namespace atlantis::testing {

TEST(FznTspAllDiff, Solve) { testModelFile("tsp_alldiff.fzn"); }

TEST(FznTspAllDiff, Gecode) { testModelFile("tsp_alldiff_gecode.fzn"); }

}  // namespace atlantis::testing