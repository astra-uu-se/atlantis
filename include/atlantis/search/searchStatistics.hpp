#pragma once

#include <memory>
#include <ostream>
#include <ranges>
#include <string>
#include <string_view>

#include "annealing/types.hpp"

namespace atlantis::search {

class Statistic {
 public:
  virtual ~Statistic() = default;
  virtual void display(std::ostream& output) const noexcept {
    output << name() << ": " << value();
  }
  [[nodiscard]] virtual std::string_view name() const noexcept = 0;
  [[nodiscard]] virtual std::string value() const noexcept = 0;
  [[nodiscard]] virtual std::unique_ptr<Statistic> clone() const = 0;
};

class CounterStatistic : public Statistic {
  const std::string _name;
  uint64_t _count{0};

 public:
  explicit CounterStatistic(std::string name) : _name(std::move(name)) {}

  void increment() { _count++; }

  [[nodiscard]] std::string_view name() const noexcept override {
    return _name;
  }
  [[nodiscard]] std::string value() const noexcept override {
    return std::to_string(_count);
  }

  [[nodiscard]] std::unique_ptr<Statistic> clone() const override {
    auto cloned = std::make_unique<CounterStatistic>(_name);
    cloned->_count = _count;
    return cloned;
  }
};

class SearchStatistics {
  std::unordered_map<std::string, std::shared_ptr<Statistic>> _statistics;
  std::optional<RoundStatistics> _roundStatistics;

 public:
  void display(std::ostream& output) const noexcept {
    for (const auto& statistic : _statistics | std::views::values) {
      statistic->display(output);
      output << std::endl;
    }
  }

  void insert(const std::shared_ptr<Statistic>& statistic) {
    const std::string name{statistic->name()};
    _statistics[name] = statistic;
  }

  std::string getValue(const std::string& name) {
    return _statistics[name]->value();
  }

  std::optional<RoundStatistics> getRoundStatistics() const {
    return _roundStatistics;
  }

  void setRoundStatistics(
      const std::optional<RoundStatistics>& roundStatistics) {
    _roundStatistics = roundStatistics;
  }
};

}  // namespace atlantis::search
