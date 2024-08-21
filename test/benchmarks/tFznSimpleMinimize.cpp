#include <string>

#include "modelTest.hpp"

namespace atlantis::testing {

TEST(FznSimpleMinimize, Solve) { testModelFile("simple_minimize.fzn"); }

TEST(FznSimpleMinimize, Gecode) { testModelFile("simple_minimize_gecode.fzn"); }

}  // namespace atlantis::testing