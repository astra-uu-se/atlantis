#include <string>

#include "modelTest.hpp"

namespace atlantis::testing {

TEST(FznTsp, Solve) { testModelFile("tsp.fzn"); }

}  // namespace atlantis::testing