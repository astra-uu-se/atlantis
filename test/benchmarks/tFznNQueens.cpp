#include <string>

#include "modelTest.hpp"

namespace atlantis::testing {

TEST(FznNQueens, Solve) {
  std::optional<std::vector<Int>> foundSolution{};
  const auto onSolution =
      [&foundSolution](
          const search::SavedAssignment& sol,
          const std::optional<
              std::vector<std::shared_ptr<search::SearchStatistics>>>&) {
        const std::vector<Int>& solution = sol.getOutputValues();
        bool validSolution = true;
        for (size_t i = 0; i < solution.size(); ++i) {
          for (size_t j = i + 1; j < solution.size(); ++j) {
            EXPECT_NE(solution[i], solution[j]) << "column";
            EXPECT_NE(solution[i] + i, solution[j] + j) << "up diagonal";
            EXPECT_NE(solution[i] - i, solution[j] - j) << "down diagonal";
            validSolution &= (solution[i] != solution[j]) &&
                             (solution[i] + i != solution[j] + j) &&
                             solution[i] - i != solution[j] - j;
          }
          if (validSolution) {
            foundSolution = solution;
          }
        }
      };
  const auto& onFinish = [&](const FznBackend::SolveOutcome outcome) {
    EXPECT_EQ(outcome == FznBackend::SolveOutcome::SATISFIABLE,
              foundSolution.has_value());
  };
  testModelFile("test/n_queens.fzn", onSolution, onFinish);
}

}  // namespace atlantis::testing