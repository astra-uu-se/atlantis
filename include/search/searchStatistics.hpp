#pragma once

#include <ostream>
#include <utility>
#include <vector>

namespace search {

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

template <typename Value>
class ValueStatistic : public Statistic {
 private:
  const std::string _name;
  Value _value;

 public:
  ValueStatistic(std::string name, Value value)
      : _name(std::move(name)), _value(std::move(value)) {}

  void display(std::ostream& output) const noexcept override {
    output << _name << ": " << _value;
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

}  // namespace search