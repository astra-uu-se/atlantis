#include <gtest/gtest.h>

#include <algorithm>
#include <boost/spirit/home/x3.hpp>
#include <boost/spirit/include/support_istream_iterator.hpp>
#include <filesystem>
#include <fstream>
#include <ranges>
#include <regex>
#include <stack>
#include <string>
#include <unordered_set>
#include <vector>

#include "atlantis/fznBackend.hpp"
#include "atlantis/logging/logger.hpp"
#include "atlantis/search/searchStatistics.hpp"

namespace atlantis::testing {

std::string ltrim(std::string s) {
  s.erase(s.begin(),
          std::ranges::find_if(s.begin(), s.end(),
                               [](unsigned char ch) { return ch != '/'; }));
  return s;
}

void logModelName(const std::string& modelPath, bool skipping, size_t index,
                  size_t total) {
  const auto padding = static_cast<int>(std::to_string(total).size());
  std::cout << (skipping ? "\033[0;33m[ SKIPPING" : "\033[0;32m[  PARSING")
            << " ] (" << std::setw(padding) << std::to_string(index + 1) << '/'
            << std::to_string(total) << ")\033[0;0m "
            << (modelPath.rfind(std::string{FZN_CHALLENGE_DIR}, 0) == 0
                    ? ltrim(modelPath.substr(
                          std::string{FZN_CHALLENGE_DIR}.size()))
                    : modelPath)
            << std::endl;
}

static void testChallenge(const std::string& fznFilePath) {
  std::filesystem::path modelFilePath(fznFilePath);
  logging::Logger logger(stdout, logging::Level::LVL_DEBUG);
  FznBackend backend(logger, std::move(modelFilePath));
  backend.setTimelimit(std::chrono::milliseconds(1000));
  const auto statistics = backend.solve(logger);
  // Don't log to std::cout, since that would interfere with MiniZinc.
  statistics.display(std::cerr);
}

class MznChallenge : public ::testing::Test {
 public:
  bool onlySmallestModel{true};

  std::string startDir = std::string(FZN_CHALLENGE_DIR) + std::string{""};

  std::unordered_set<std::string> malloc{};

  std::unordered_set<std::string> timeout{};

  std::unordered_set<std::string> unbrokenCycles{
      std::string(FZN_CHALLENGE_DIR) + std::string{"/mqueens"},
      std::string(FZN_CHALLENGE_DIR) + std::string{"/pattern-set-mining"},
      std::string(FZN_CHALLENGE_DIR) + std::string{"/project-planning"},
      std::string(FZN_CHALLENGE_DIR) + std::string{"/tdtsp"}};

  std::unordered_set<std::string> unsatAllEqual{std::string(FZN_CHALLENGE_DIR) +
                                                std::string{"/still_life"}};

  std::unordered_set<std::string> failing{};

  std::unordered_set<std::string> unbounded{
      std::string{FZN_CHALLENGE_DIR} + std::string("/2DBinPacking"),
      std::string{FZN_CHALLENGE_DIR} + std::string("/wwtpp-random"),
      std::string{FZN_CHALLENGE_DIR} + std::string("/wwtpp-real")};

  std::vector<std::string> passingFznModels;
  std::vector<std::string> mallocFznModels;
  std::vector<std::string> timeoutFznModels;
  std::vector<std::string> unbrokenCycleFznModels;
  std::vector<std::string> unsatAllEqualFznModels;
  std::vector<std::string> failingFznModels;
  std::vector<std::string> unboundedFznModels;

  void SetUp() override {
    std::stack<std::string> stack;
    stack.emplace(FZN_CHALLENGE_DIR);
    std::unordered_set<std::string> visited{std::string(FZN_CHALLENGE_DIR)};
    std::unordered_map<std::string, std::vector<std::pair<size_t, std::string>>>
        fznModelsByDir;

    while (!stack.empty()) {
      const std::string dir = stack.top();
      stack.pop();
      if (!std::filesystem::exists(dir)) {
        continue;
      }
      for (const auto& entry : std::filesystem::directory_iterator(dir)) {
        if (entry.is_directory()) {
          if (!visited.contains(entry.path().string())) {
            stack.push(entry.path().string());
            visited.insert(entry.path().string());
          }
        } else if (entry.is_regular_file() &&
                   entry.path().extension() == ".fzn") {
          const std::string dirPath = entry.path().parent_path().string();
          if (!fznModelsByDir.contains(dirPath)) {
            fznModelsByDir.emplace(
                dirPath, std::vector<std::pair<size_t, std::string>>{
                             {entry.file_size(), entry.path().string()}});
          } else {
            fznModelsByDir.at(dirPath).emplace_back(entry.file_size(),
                                                    entry.path().string());
          }
        }
      }
    }

    std::vector<std::string> dirs;
    dirs.reserve(fznModelsByDir.size());

    for (auto& [dir, fznModels] : fznModelsByDir) {
      std::ranges::sort(fznModels.begin(), fznModels.end());
      dirs.emplace_back(dir);
    }

    EXPECT_EQ(dirs.size(), fznModelsByDir.size());

    std::ranges::sort(dirs.begin(), dirs.end());

    passingFznModels.clear();
    failingFznModels.clear();

    mallocFznModels.reserve(malloc.size());
    timeoutFznModels.reserve(timeout.size());
    unbrokenCycleFznModels.reserve(unbrokenCycles.size());
    unsatAllEqualFznModels.reserve(unsatAllEqual.size());
    unboundedFznModels.reserve(unbounded.size());
    failingFznModels.reserve(failing.size());
    passingFznModels.reserve(fznModelsByDir.size() - malloc.size() -
                             timeout.size() - unbrokenCycles.size() -
                             unsatAllEqual.size() - failing.size() -
                             unbounded.size());

    for (const auto& dirPath : dirs) {
      EXPECT_TRUE(fznModelsByDir.contains(dirPath));
      EXPECT_GT(fznModelsByDir.at(dirPath).size(), 0);
      for (const auto& fznModel :
           std::views::values(fznModelsByDir.at(dirPath))) {
        if (malloc.contains(dirPath)) {
          mallocFznModels.emplace_back(fznModel);
        } else if (timeout.contains(dirPath)) {
          timeoutFznModels.emplace_back(fznModel);
        } else if (unbrokenCycles.contains(dirPath)) {
          unbrokenCycleFznModels.emplace_back(fznModel);
        } else if (unsatAllEqual.contains(dirPath)) {
          unsatAllEqualFznModels.emplace_back(fznModel);
        } else if (failing.contains(dirPath)) {
          failingFznModels.emplace_back(fznModel);
        } else if (unbounded.contains(dirPath)) {
          unboundedFznModels.emplace_back(fznModel);
        } else {
          passingFznModels.emplace_back(fznModel);
        }
        if (onlySmallestModel) {
          break;
        }
      }
    }
  }
};

TEST_F(MznChallenge, DISABLED_passing) {
  for (size_t i = 0; i < passingFznModels.size(); ++i) {
    if (passingFznModels.at(i) < startDir) {
      logModelName(passingFznModels.at(i), true, i, passingFznModels.size());
    } else {
      logModelName(passingFznModels.at(i), false, i, passingFznModels.size());
      testChallenge(passingFznModels.at(i));
    }
  }
}

TEST_F(MznChallenge, DISABLED_malloc) {
  for (size_t i = 0; i < mallocFznModels.size(); ++i) {
    logModelName(mallocFznModels.at(i), false, i, mallocFznModels.size());
    testChallenge(mallocFznModels.at(i));
  }
}
TEST_F(MznChallenge, DISABLED_timeout) {
  for (size_t i = 0; i < timeout.size(); ++i) {
    logModelName(timeoutFznModels.at(i), false, i, timeoutFznModels.size());
    testChallenge(timeoutFznModels.at(i));
  }
}
TEST_F(MznChallenge, DISABLED_unbrokenCycle) {
  for (size_t i = 0; i < unbrokenCycleFznModels.size(); ++i) {
    logModelName(unbrokenCycleFznModels.at(i), false, i,
                 unbrokenCycleFznModels.size());
    testChallenge(unbrokenCycleFznModels.at(i));
  }
}
TEST_F(MznChallenge, DISABLED_unsatAllEqual) {
  for (size_t i = 0; i < unsatAllEqualFznModels.size(); ++i) {
    logModelName(unsatAllEqualFznModels.at(i), false, i,
                 unsatAllEqualFznModels.size());
    testChallenge(unsatAllEqualFznModels.at(i));
  }
}
TEST_F(MznChallenge, DISABLED_failing) {
  for (size_t i = 0; i < failingFznModels.size(); ++i) {
    logModelName(failingFznModels.at(i), false, i, failingFznModels.size());
    testChallenge(failingFznModels.at(i));
  }
}

}  // namespace atlantis::testing