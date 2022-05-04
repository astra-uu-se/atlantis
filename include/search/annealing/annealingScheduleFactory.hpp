#pragma once

#include <filesystem>
#include <optional>
#include <utility>

#include "annealingSchedule.hpp"

namespace search {

class AnnealingScheduleCreationError : public std::exception {
 private:
  const std::string _msg;

 public:
  explicit AnnealingScheduleCreationError(std::string msg) : _msg(std::move(msg)) {}
  explicit AnnealingScheduleCreationError(const char* msg) : _msg(msg) {}

  [[nodiscard]] const char* what() const noexcept override { return _msg.c_str(); }
};

class AnnealingScheduleFactory {
 private:
  std::optional<std::filesystem::path> _scheduleDefinition;

 public:
  explicit AnnealingScheduleFactory(
      std::optional<std::filesystem::path> scheduleDefinition = {})
      : _scheduleDefinition(std::move(scheduleDefinition)) {}

  /**
   * Creates an annealing schedule based on the definition file provided to
   * the constructor of this class. If no definition file was provided, a
   * default schedule is used.
   *
   * @return The annealing schedule to use for the search.
   * @throws AnnealingScheduleCreationError If no schedule can be created from
   * the given definition file.
   */
  [[nodiscard]] std::unique_ptr<AnnealingSchedule> create() const;
};

}  // namespace search
