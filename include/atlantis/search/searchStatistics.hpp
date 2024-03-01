#pragma once

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
  };
  virtual std::string_view name() const noexcept = 0;
  virtual std::string value() const noexcept = 0;
};

class CounterStatistic : public Statistic {
 private:
  const std::string _name;
  uint64_t _count{0};

 public:
  explicit CounterStatistic(std::string name) : _name(std::move(name)) {}

  void increment() { _count++; }
  std::string_view name() const noexcept override { return _name; }
  std::string value() const noexcept override { return std::to_string(_count); }
};

class SearchStatistics {
 private:
  std::vector<std::unique_ptr<Statistic>> _statistics;

 public:
  using const_iterator =
      std::vector<std::unique_ptr<Statistic>>::const_iterator;

  SearchStatistics() {}
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
