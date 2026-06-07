#include <string>

#include "modelTest.hpp"

namespace atlantis::testing {

TEST(FznSimpleMinimize, Solve) { testModelFile("simple_minimize.fzn"); }

}  // namespace atlantis::testing