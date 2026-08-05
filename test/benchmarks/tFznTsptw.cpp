#include <string>

#include "modelTest.hpp"

namespace atlantis::testing {

TEST(FznTsptw, Solve) {
  const std::unordered_set<Int> validObjectives{
      153, 160, 162, 165, 171, 174, 177, 180, 187, 194, 195, 198, 199, 201};
  testModelFile("test/tsptw_6.fzn", validObjectives);
}

}  // namespace atlantis::testing
