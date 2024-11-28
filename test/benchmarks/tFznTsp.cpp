#include <string>

#include "modelTest.hpp"

namespace atlantis::testing {

TEST(FznTsp, Solve) { testModelFile("tsp.fzn"); }

TEST(FznTsp, Gecode) { testModelFile("tsp_gecode.fzn"); }

}  // namespace atlantis::testing