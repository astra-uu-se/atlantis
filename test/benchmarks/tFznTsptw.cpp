#include <string>

#include "modelTest.hpp"

namespace atlantis::testing {

std::unordered_set<Int> validObjectives() {
  return std::unordered_set<Int>{153, 160, 162, 165, 171, 174, 177,
                                 180, 187, 194, 195, 198, 199, 201};
}

TEST(FznTsptw, Solve) { testModelFile("tsptw_6.fzn", validObjectives()); }

}  // namespace atlantis::testing