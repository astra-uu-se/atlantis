#include <string>

#include "modelTest.hpp"

namespace atlantis::testing {

TEST(FznCompDomain, Solve) {
  const std::vector<std::vector<Int>> expectedOutputs{
      {1, 1, 2, 4}, {1, 1, 4, 2}, {1, 2, 1, 4}, {1, 2, 4, 1},
      {1, 4, 1, 2}, {1, 4, 2, 1}, {2, 1, 1, 4}, {2, 1, 4, 1},
      {2, 4, 1, 1}, {4, 1, 1, 2}, {4, 1, 2, 1}, {4, 2, 1, 1}};

  testModelFile("test/comp_domain_ann.fzn", expectedOutputs);
}

}  // namespace atlantis::testing