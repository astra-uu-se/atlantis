#include <string>

#include "modelTest.hpp"

namespace atlantis::testing {

TEST(FznVarSet, Solve) { testModelFile("test/var_set_2.fzn"); }
}  // namespace atlantis::testing