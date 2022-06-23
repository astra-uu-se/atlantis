#pragma once

#include <numeric>
#include <variant>
#include <vector>

#include "exceptions/exceptions.hpp"
#include "types.hpp"

/**
 * Models the domain of a variable. Since this might be a large object managing
 * heap memory, the copy constructor is deleted.
 */
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
  [[nodiscard]] bool isConstant() { return size() == 1; };

  [[nodiscard]] virtual bool contains(Int val) const = 0;

  /**
   * removes all values in the domain that are less than, but not equal
   * to, the input value.
   */
  virtual void removeBelow(Int value) = 0;

  /**
   * removes all values in the domain that are greater than, but not
   * equal to, the input value.
   */
  virtual void removeAbove(Int value) = 0;

  /**
   * Fixes the domain to a single value
   */
  virtual void fix(Int value) = 0;

  /**
   * @return if the domain is not a superset of lb..ub,
   * then returns the relative complement of lb..ub in domain,
   * otherwise an empty vector is returned.
   */
  [[nodiscard]] virtual std::vector<DomainEntry> relativeComplementIfIntersects(
      const Int lb, const Int ub) const = 0;
};

class IntervalDomain : public Domain {
 private:
  Int _lb;
  Int _ub;

  std::vector<Int> _values;
  bool _valuesCollected{false};

 public:
  IntervalDomain(Int lb, Int ub);

  [[nodiscard]] Int lowerBound() const noexcept override;
  [[nodiscard]] Int upperBound() const noexcept override;
  [[nodiscard]] std::pair<Int, Int> bounds() const noexcept override;
  [[nodiscard]] size_t size() const noexcept override;
  [[nodiscard]] std::vector<DomainEntry> relativeComplementIfIntersects(
      const Int lb, const Int ub) const override;
  void setLowerBound(Int lb);
  void setUpperBound(Int ub);

  bool contains(Int value) const override;

  void removeBelow(Int value) override;

  void removeAbove(Int value) override;

  void fix(Int value) override;

  bool operator==(const IntervalDomain& other) const;

  bool operator!=(const IntervalDomain& other) const;
};

class SetDomain : public Domain {
 private:
  std::vector<Int> _values;

 public:
  explicit SetDomain(std::vector<Int> values);

  [[nodiscard]] const std::vector<Int>& values() const;
  [[nodiscard]] Int lowerBound() const noexcept override;

  [[nodiscard]] Int upperBound() const noexcept override;

  [[nodiscard]] std::pair<Int, Int> bounds() const noexcept override;

  [[nodiscard]] size_t size() const noexcept override;

  std::vector<DomainEntry> relativeComplementIfIntersects(
      const Int lb, const Int ub) const override;

  void removeValue(Int value);

  bool contains(Int value) const override;

  void removeBelow(Int value) override;

  void removeAbove(Int value) override;

  void fix(Int value) override;

  bool operator==(const SetDomain& other) const;

  bool operator!=(const SetDomain& other) const;
};

class SearchDomain : public Domain {
 private:
  std::variant<IntervalDomain, SetDomain> _domain;

 public:
  explicit SearchDomain(std::vector<Int> values);
  explicit SearchDomain(Int lb, Int ub);

  [[nodiscard]] inline const std::variant<IntervalDomain, SetDomain>&
  innerDomain() const noexcept {
    return _domain;
  }

  [[nodiscard]] const std::vector<Int>& values();

  [[nodiscard]] Int lowerBound() const noexcept override;

  [[nodiscard]] Int upperBound() const noexcept override;

  [[nodiscard]] std::pair<Int, Int> bounds() const noexcept override;

  [[nodiscard]] size_t size() const noexcept override;

  bool contains(Int value) const override;

  void removeBelow(Int value) override;

  void removeAbove(Int value) override;

  std::vector<DomainEntry> relativeComplementIfIntersects(
      const Int lb, const Int ub) const override;

  void removeValue(Int value);

  void fix(Int value) override;

  bool operator==(const SearchDomain& other) const;

  bool operator!=(const SearchDomain& other) const;
};
