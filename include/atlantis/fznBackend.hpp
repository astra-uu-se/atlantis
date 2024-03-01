#pragma once

#include <cstdint>
#include <filesystem>
#include <iostream>
#include <optional>

#include "atlantis/logging/logger.hpp"
#include "atlantis/search/annealing/annealingScheduleFactory.hpp"
#include "atlantis/search/searchStatistics.hpp"

namespace atlantis {

class FznBackend {
 private:
  std::filesystem::path _modelFile;
  search::AnnealingScheduleFactory _annealingScheduleFactory;
  std::optional<std::chrono::milliseconds> _timeout;
  std::uint_fast32_t _seed;

 public:
  explicit FznBackend(
      std::filesystem::path&& modelFile,
      search::AnnealingScheduleFactory&& annealingScheduleFactory,
      std::chrono::milliseconds timeout);
  FznBackend(std::filesystem::path&& modelFile,
             search::AnnealingScheduleFactory&& annealingScheduleFactory,
             std::uint_fast32_t seed,
             std::optional<std::chrono::milliseconds> timeout);

  search::SearchStatistics solve(logging::Logger& logger);
};

}  // namespace atlantis
