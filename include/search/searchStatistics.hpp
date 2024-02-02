#pragma once

#include <ostream>
#include <utility>
#include <vector>

namespace atlantis::search {

class Statistic {
 public:
  virtual ~Statistic() = default;
  virtual void display(std::ostream& output) const noexcept = 0;
};

class CounterStatistic : public Statistic {
 private:
  const std::string _name;
  uint64_t _count{0};

 public:
  explicit CounterStatistic(std::string name) : _name(std::move(name)) {}

  void increment() { _count++; }
  void display(std::ostream& output) const noexcept override {
    output << _name << ": " << _count;
  }
};

class SearchStatistics : public Statistic {
 private:
  std::vector<std::unique_ptr<Statistic>> _statistics;

 public:
  explicit SearchStatistics(std::vector<std::unique_ptr<Statistic>> statistics)
      : _statistics(std::move(statistics)) {}

  void display(std::ostream& output) const noexcept override {
    for (const auto& statistic : _statistics) {
      statistic->display(output);
      output << std::endl;
    }
  }
};

}  // namespace atlantis::search