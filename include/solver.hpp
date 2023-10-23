#pragma once

#include <cstdint>
#include <filesystem>
#include <optional>
#include <utility>

#include "logging/logger.hpp"
#include "search/annealing/annealingScheduleFactory.hpp"
#include "search/searchStatistics.hpp"

namespace atlantis {

class Solver {
 private:
  std::filesystem::path _modelFile;
  search::AnnealingScheduleFactory _annealingScheduleFactory;
  std::optional<std::chrono::milliseconds> _timeout;
  std::uint_fast32_t _seed;

 public:
  explicit Solver(std::filesystem::path modelFile,
                  search::AnnealingScheduleFactory annealingScheduleFactory,
                  std::chrono::milliseconds timeout);
  Solver(std::filesystem::path modelFile,
         search::AnnealingScheduleFactory annealingScheduleFactory,
         std::uint_fast32_t seed,
         std::optional<std::chrono::milliseconds> timeout)
      : _modelFile(std::move(modelFile)),
        _annealingScheduleFactory(std::move(annealingScheduleFactory)),
        _timeout(timeout),
        _seed(seed) {}

  search::SearchStatistics solve(logging::Logger& logger);
};

}  // namespace atlantis