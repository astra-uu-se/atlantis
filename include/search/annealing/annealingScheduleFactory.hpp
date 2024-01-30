#pragma once

#include <filesystem>
#include <fstream>
#include <optional>
#include <string>
#include <utility>

#include "annealingSchedule.hpp"
#include "search/annealing/annealerContainer.hpp"

namespace atlantis::search {

class AnnealingScheduleCreationError : public std::exception {
 private:
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
 private:
  std::optional<std::filesystem::path> _scheduleDefinition;

  [[nodiscard]] static inline std::unique_ptr<AnnealingSchedule> defaultAnnealingSchedule() {
    std::vector<std::unique_ptr<AnnealingSchedule>> inner;
    inner.push_back(AnnealerContainer::heating(1.2, 0.75));
    inner.push_back(AnnealerContainer::cooling(0.99, 4));
    return AnnealerContainer::loop(
        AnnealerContainer::sequence(std::move(inner)), 5);
  }

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

}  // namespace atlantis::search
