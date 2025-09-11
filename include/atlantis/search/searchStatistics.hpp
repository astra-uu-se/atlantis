#pragma once

#include <memory>
#include <ostream>
#include <string>
#include <string_view>
#include <vector>

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
  std::vector<std::unique_ptr<Statistic>> _statistics;

 public:
  using const_iterator =
      std::vector<std::unique_ptr<Statistic>>::const_iterator;

  SearchStatistics() = default;
  explicit SearchStatistics(std::vector<std::unique_ptr<Statistic>> statistics)
      : _statistics(std::move(statistics)) {}

  void display(std::ostream& output) const noexcept {
    for (const auto& statistic : _statistics) {
      statistic->display(output);
      output << std::endl;
    }
  }

  const_iterator begin() { return _statistics.begin(); }
  const_iterator end() { return _statistics.end(); }
};

}  // namespace atlantis::search
