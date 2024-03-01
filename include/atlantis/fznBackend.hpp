#pragma once

#include <cstdint>
#include <filesystem>
#include <fznparser/model.hpp>
#include <iostream>
#include <optional>

#include "atlantis/logging/logger.hpp"
#include "atlantis/search/annealing/annealingScheduleFactory.hpp"
#include "atlantis/search/searchStatistics.hpp"

namespace atlantis {

class FznBackend {
 private:
  fznparser::Model _model;
  search::AnnealingScheduleFactory _annealingScheduleFactory;
  std::optional<std::chrono::milliseconds> _timeout;
  std::uint_fast32_t _seed;

 public:
  FznBackend(fznparser::Model&& model,
             search::AnnealingScheduleFactory&& annealingScheduleFactory,
             std::uint_fast32_t seed,
             std::optional<std::chrono::milliseconds> timeout)
      : _model(std::move(model)),
        _annealingScheduleFactory(std::move(annealingScheduleFactory)),
        _timeout(timeout),
        _seed(seed){};

  FznBackend(logging::Logger& logger, std::filesystem::path&& modelFile,
             search::AnnealingScheduleFactory&& annealingScheduleFactory,
             std::uint_fast32_t seed,
             std::optional<std::chrono::milliseconds> timeout);
  explicit FznBackend(
      logging::Logger& logger, std::filesystem::path&& modelFile,
      search::AnnealingScheduleFactory&& annealingScheduleFactory,
      std::chrono::milliseconds timeout);

  search::SearchStatistics solve(logging::Logger& logger);
};

}  // namespace atlantis
