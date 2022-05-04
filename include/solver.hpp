#pragma once

#include <cstdint>
#include <filesystem>
#include <optional>
#include <utility>

#include "search/searchStatistics.hpp"

class Solver {
 private:
  std::filesystem::path _modelFile;
  std::optional<std::chrono::milliseconds> _timeout;
  std::uint_fast32_t _seed;

 public:
  explicit Solver(std::filesystem::path modelFile,
                  std::chrono::milliseconds timeout);
  Solver(std::filesystem::path modelFile, std::uint_fast32_t seed,
         std::optional<std::chrono::milliseconds> timeout)
      : _modelFile(std::move(modelFile)), _timeout(timeout), _seed(seed) {}

  search::SearchStatistics solve();
};
