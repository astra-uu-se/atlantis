#pragma once

#include <variant>
#include <vector>

#include "types.hpp"

class Domain {
 public:
  virtual ~Domain() = default;

  /**
   * @return The value of the smallest element in the domain.
   */
  [[nodiscard]] virtual Int lowerBound() const noexcept = 0;

  /**
   * @return The value of the largest element in the domain.
   */
  [[nodiscard]] virtual Int upperBound() const noexcept = 0;

  /**
   * @return The values of the lowest and largest elements in the domain.
   */
  [[nodiscard]] virtual std::pair<Int, Int> bounds() const noexcept = 0;

  /**
   * @return The number of values of the domain.
   */
  [[nodiscard]] virtual size_t size() const noexcept = 0;

  /**
   * @return The value of the largest element in the domain.
   */
  [[nodiscard]] inline bool isConstant() const { return size() == 1; };

  /**
   * @return if the domain is not a superset of lb..ub,
   * then returns the relative complement of lb..ub in domain,
   * otherwise an empty vector is returned.
   */
  [[nodiscard]] virtual std::vector<DomainEntry> relativeComplementIfIntersects(
      Int lb, Int ub) const = 0;
};

class IntervalDomain : public Domain {
 private:
  Int _lb;
  Int _ub;

 public:
  IntervalDomain(Int lb, Int ub) : _lb(lb), _ub(ub) {}

  [[nodiscard]] Int lowerBound() const noexcept override;
  [[nodiscard]] Int upperBound() const noexcept override;
  [[nodiscard]] std::pair<Int, Int> bounds() const noexcept override;
  [[nodiscard]] size_t size() const noexcept override;

  [[nodiscard]] std::vector<DomainEntry> relativeComplementIfIntersects(
      Int lb, Int ub) const override;

  void setLowerBound(Int lb);
  void setUpperBound(Int ub);

  bool operator==(const IntervalDomain& other) const;
  bool operator!=(const IntervalDomain& other) const;
};

class SetDomain : public Domain {
 private:
  std::vector<Int> _values;

 public:
  explicit SetDomain(std::vector<Int> values);

  [[nodiscard]] const std::vector<Int>& values() const { return _values; }

  [[nodiscard]] Int lowerBound() const noexcept override;
  [[nodiscard]] Int upperBound() const noexcept override;
  [[nodiscard]] std::pair<Int, Int> bounds() const noexcept override;
  [[nodiscard]] size_t size() const noexcept override;

  [[nodiscard]] std::vector<DomainEntry> relativeComplementIfIntersects(
      Int lb, Int ub) const override;

  void removeValue(Int value);

  bool operator==(const SetDomain& other) const;
  bool operator!=(const SetDomain& other) const;
};

class SearchDomain : public Domain {
 private:
  std::variant<IntervalDomain, SetDomain> _domain;

 public:
  explicit SearchDomain(std::vector<Int> values)
      : _domain(SetDomain{std::move(values)}) {}
  explicit SearchDomain(Int lb, Int ub) : _domain(IntervalDomain{lb, ub}) {}

  [[nodiscard]] const std::variant<IntervalDomain, SetDomain>& innerDomain()
      const noexcept;

  [[nodiscard]] const std::vector<Int>& values();

  [[nodiscard]] Int lowerBound() const noexcept override;
  [[nodiscard]] Int upperBound() const noexcept override;
  [[nodiscard]] std::pair<Int, Int> bounds() const noexcept override;
  [[nodiscard]] size_t size() const noexcept override;

  [[nodiscard]] std::vector<DomainEntry> relativeComplementIfIntersects(
      Int lb, Int ub) const override;

  void removeValue(Int value);

  bool operator==(const SearchDomain& other) const;
  bool operator!=(const SearchDomain& other) const;
};
