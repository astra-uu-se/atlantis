#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

#include "AnnealingScheduleContainerFactory.hpp"

namespace atlantis::search {

class AnnealingSchedule;

class AnnealingScheduleCreationError : public std::exception {
  const std::string _msg;

 public:
  explicit AnnealingScheduleCreationError(std::string msg)
      : _msg(std::move(msg)) {}
  explicit AnnealingScheduleCreationError(const char* msg) : _msg(msg) {}

  [[nodiscard]] const char* what() const noexcept override {
    return _msg.c_str();
  }
};

class AnnealingScheduleFactory {
  std::optional<std::filesystem::path> _scheduleDefinition;
  std::vector<std::unique_ptr<AnnealingScheduleContainerFactory>> _factories;
  size_t _armCount = 1;

  inline void setDefaultAnnealingSchedule();

 public:
  explicit AnnealingScheduleFactory(
      const std::optional<std::filesystem::path>& scheduleDefinition = {});

  void SetAnnealingSchedule(const std::filesystem::path& scheduleDefinition);

  /**
   * Creates an annealing schedule based on the definition file provided to
   * the constructor of this class. If no definition file was provided, a
   * default schedule is used.
   *
   * @return The annealing schedule to use for the search.
   * @throws AnnealingScheduleCreationError If no schedule can be created from
   * the given definition file.
   */
  [[nodiscard]] std::unique_ptr<AnnealingSchedule> create(size_t index) const;

  [[nodiscard]] size_t armCount() const{
    return _armCount;
  }
};

}  // namespace atlantis::search
