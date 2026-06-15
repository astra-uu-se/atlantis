#include <string>

#include "modelTest.hpp"

namespace atlantis::testing {

static std::unordered_set<Int> solutions() {
  const Int capacity = 269;
  const std::vector<Int> weights{95, 4, 60, 32, 23, 72, 80, 62, 65, 46};
  // we are maximizing, the objective has its sign inverted:
  const std::vector<Int> gains{-55, -10, -47, -5, -4, -50, -8, -61, -85, -87};
  std::vector<std::vector<std::vector<Int>>> matrix(
      weights.size(),
      std::vector<std::vector<Int>>(capacity + 1, std::vector<Int>()));
  for (size_t i = 0; i < weights.size(); ++i) {
    matrix.at(i).at(0).emplace_back(0);
  }
  if (weights.at(0) <= capacity) {
    matrix.at(0).at(weights.at(0)).emplace_back(gains.at(0));
  }
  for (size_t i = 1; i < weights.size(); ++i) {
    for (Int c = 0; c <= capacity; ++c) {
      matrix.at(i).at(c) = matrix.at(i - 1).at(c);
      if (weights.at(i) <= c) {
        for (const Int g : matrix.at(i - 1).at(c - weights.at(i))) {
          matrix.at(i).at(c).emplace_back(g + gains.at(i));
        }
      }
    }
  }
  std::unordered_set<Int> solutions;
  for (Int c = 0; c <= capacity; ++c) {
    for (Int g : matrix.back().at(c)) {
      solutions.insert(g);
    }
  }
  return solutions;
}

TEST(FznKnapsack, DISABLED_Solve) {
  testModelFile("knapsack/f1_l-d_kp_10_269.fzn", solutions());
}

TEST(FznKnapsack, DISABLED_SolveBool) {
  testModelFile("knapsack/f1_l-d_kp_10_269_bool.fzn", solutions());
}

}  // namespace atlantis::testing