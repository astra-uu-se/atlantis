#include <string>

#include "modelTest.hpp"

namespace atlantis::testing {

TEST(FznTspAllDiff, Solve) { testModelFile("tsp_alldiff.fzn"); }

}