#pragma once

#include <cstdint>
#include <filesystem>
#include <fznparser/model.hpp>
#include <iostream>
#include <optional>

#include "atlantis/invariantgraph/fznInvariantGraph.hpp"
#include "atlantis/logging/logger.hpp"
#include "atlantis/search/annealing/annealingScheduleFactory.hpp"
#include "atlantis/search/assignment.hpp"
#include "atlantis/search/searchStatistics.hpp"

namespace atlantis {

class FznBackend {
 public:
  static void onSolutionDefault(const invariantgraph::FznInvariantGraph&,
                                const search::Assignment&);
  static void onFinishDefault(bool);

 private:
  fznparser::Model _model;
  search::AnnealingScheduleFactory _annealingScheduleFactory;
  std::optional<std::chrono::milliseconds> _timelimit;
  std::uint_fast32_t _seed;
  std::optional<std::filesystem::path> _dotFilePath{};

  std::function<void(const invariantgraph::FznInvariantGraph&,
                     const search::Assignment&)>
      _onSolution = onSolutionDefault;
  std::function<void(bool)> _onFinish = onFinishDefault;

 public:
  FznBackend(fznparser::Model&& model)
      : _model(std::move(model)), _seed(std::time(nullptr)) {}

  FznBackend(logging::Logger& logger, std::filesystem::path&& modelFile);

  search::SearchStatistics solve(logging::Logger& logger);

  void setTimelimit(std::optional<std::chrono::milliseconds> timeLimit) {
    _timelimit = timeLimit;
  }

  void setAnnealingScheduleFactory(search::AnnealingScheduleFactory&& factory) {
    _annealingScheduleFactory = factory;
  }

  void setRandomSeed(std::uint_fast32_t seed) { _seed = seed; }

  void setOnSolution(
      std::function<void(const invariantgraph::FznInvariantGraph&,
                         const search::Assignment&)>
          onSolution) {
    _onSolution = onSolution;
  }

  void setDotFilePath(std::filesystem::path&& path) {
    _dotFilePath = std::optional<std::filesystem::path>(std::move(path));
  }

  void setOnFinish(std::function<void(bool)> onFinish) { _onFinish = onFinish; }
};

}  // namespace atlantis
