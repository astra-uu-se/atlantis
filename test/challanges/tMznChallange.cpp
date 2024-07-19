#include <gtest/gtest.h>

#include <algorithm>
#include <boost/spirit/home/x3.hpp>
#include <boost/spirit/include/support_istream_iterator.hpp>
#include <filesystem>
#include <fstream>
#include <regex>
#include <stack>
#include <string>
#include <unordered_set>
#include <vector>

#include "atlantis/fznBackend.hpp"

namespace atlantis::testing {

std::string ltrim(std::string s) {
  s.erase(s.begin(), std::find_if(s.begin(), s.end(),
                                  [](unsigned char ch) { return ch != '/'; }));
  return s;
}

void logModelName(const std::string& modelPath, bool skipping, size_t index,
                  size_t total) {
  size_t padding = std::to_string(total).size();
  std::cout << (skipping ? "\033[0;33m[ SKIPPING" : "\033[0;32m[  PARSING")
            << " ] (" << std::setw(padding) << std::to_string(index + 1) << '/'
            << std::to_string(total) << ")\033[0;0m "
            << (modelPath.rfind(std::string{FZN_CHALLENGE_DIR}, 0) == 0
                    ? ltrim(modelPath.substr(
                          std::string{FZN_CHALLENGE_DIR}.size()))
                    : modelPath)
            << std::endl;
}

static void testChallange(const std::string& fznFilePath) {
  std::filesystem::path modelFilePath(fznFilePath);
  logging::Logger logger(stdout, logging::Level::ERR);
  FznBackend backend(logger, std::move(modelFilePath));
  backend.setTimelimit(std::chrono::milliseconds(1000));
  auto statistics = backend.solve(logger);
  // Don't log to std::cout, since that would interfere with MiniZinc.
  statistics.display(std::cerr);
}

TEST(MznChallange, run) {
  std::stack<std::string> stack;
  stack.emplace(FZN_CHALLENGE_DIR);
  std::unordered_set<std::string> visited{std::string(FZN_CHALLENGE_DIR)};
  std::vector<std::string> fznModels;

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
        fznModels.push_back(entry.path().string());
      }
    }
  }

  std::sort(fznModels.begin(), fznModels.end());

  std::vector<bool> modelsToSkip(fznModels.size(), false);
  visited.clear();

  visited.emplace(std::string(FZN_CHALLENGE_DIR + std::string{"/cargo"}));
  visited.emplace(
      std::string(FZN_CHALLENGE_DIR + std::string{"/carpet-cutting"}));
  visited.emplace(
      std::string(FZN_CHALLENGE_DIR + std::string{"/cryptanalysis"}));

  for (size_t i = 0; i < fznModels.size(); ++i) {
    std::filesystem::path modelPath(fznModels.at(i));
    EXPECT_TRUE(modelPath.has_parent_path());
    std::filesystem::path parentPath = modelPath.parent_path();
    std::string dirPath = parentPath.string();
    if (visited.contains(dirPath)) {
      modelsToSkip.at(i) = true;
    } else {
      visited.insert(dirPath);
    }
  }

  for (size_t i = 0; i < fznModels.size(); ++i) {
    const bool skipping = modelsToSkip.at(i);
    logModelName(fznModels.at(i), skipping, i, fznModels.size());
    if (!skipping) {
      testChallange(fznModels.at(i));
    }
  }
}

}  // namespace atlantis::testing