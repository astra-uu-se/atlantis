#include <string>

#include "modelTest.hpp"

namespace atlantis::testing {

static void knapsackSolutions(const search::SavedAssignment& sol,
                              const Int trueVal,
                              std::optional<Int>& foundOptimum) {
  EXPECT_FALSE(sol.cost().hasViolation());

  constexpr Int capacity = 269;
  constexpr std::array<Int, 10> weight{95, 4, 60, 32, 23, 72, 80, 62, 65, 46};
  constexpr std::array<Int, 10> profit{55, 10, 47, 5, 4, 50, 8, 61, 85, 87};
  const std::vector<Int>& outputs = sol.getOutputValues();
  Int totalWeight = 0;
  Int totalProfit = 0;
  EXPECT_EQ(outputs.size(), weight.size());
  for (size_t i = 0; i < outputs.size(); ++i) {
    totalWeight += outputs.at(i) == trueVal ? weight[i] : 0;
    totalProfit += outputs.at(i) == trueVal ? profit[i] : 0;
  }
  const Int actualProfit = std::abs(sol.cost().objective());
  EXPECT_LE(totalWeight, capacity);
  EXPECT_EQ(totalProfit, actualProfit);
  if (!sol.cost().hasViolation() && totalWeight <= capacity &&
      totalProfit == actualProfit) {
    foundOptimum = actualProfit;
  }
}

TEST(FznKnapsack, Solve) {
  std::optional<Int> foundOptimum{};
  const auto onSolution =
      [&foundOptimum](
          const search::SavedAssignment& sol,
          const std::optional<
              std::vector<std::shared_ptr<search::SearchStatistics>>>&) {
        knapsackSolutions(sol, 1, foundOptimum);
      };
  const auto& onFinish =
      [&foundOptimum](const FznBackend::SolveOutcome outcome) {
        EXPECT_EQ(outcome == FznBackend::SolveOutcome::SATISFIABLE,
                  foundOptimum.has_value());
      };
  testModelFile("test/f1_l-d_kp_10_269.fzn", onSolution, onFinish);
}

TEST(FznKnapsack, SolveBool) {
  std::optional<Int> foundOptimum{};
  const auto onSolution =
      [&foundOptimum](
          const search::SavedAssignment& sol,
          const std::optional<
              std::vector<std::shared_ptr<search::SearchStatistics>>>&) {
        knapsackSolutions(sol, 0, foundOptimum);
      };
  const auto& onFinish =
      [&foundOptimum](const FznBackend::SolveOutcome outcome) {
        EXPECT_EQ(outcome == FznBackend::SolveOutcome::SATISFIABLE,
                  foundOptimum.has_value());
      };
  testModelFile("test/f1_l-d_kp_10_269_bool.fzn", onSolution, onFinish);
}

}  // namespace atlantis::testing