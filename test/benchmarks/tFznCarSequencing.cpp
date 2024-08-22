#include <string>

#include "modelTest.hpp"

namespace atlantis::testing {

TEST(FznCarSequencing, Solve) { testModelFile("car_sequencing.fzn"); }

TEST(FznCarSequencing, Gecode) { testModelFile("car_sequencing_gecode.fzn"); }

}  // namespace atlantis::testing