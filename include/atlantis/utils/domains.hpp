#pragma once

#include <variant>
#include <vector>

#include "atlantis/types.hpp"

namespace atlantis {

class SetDomain;
class IntervalDomain;

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
  [[nodiscard]] virtual Int lowerBound() const = 0;

  /**
   * @return The value of the largest element in the domain.
   */
  [[nodiscard]] virtual Int upperBound() const = 0;

  /**
   * @return The values of the lowest and largest elements in the domain.
   */
  [[nodiscard]] virtual std::pair<Int, Int> bounds() const = 0;

  /**
   * @return The number of values of the domain.
   */
  [[nodiscard]] virtual size_t size() const noexcept = 0;

  /**
   * @return true if the domain contains exactly one value, else false.
   */
  [[nodiscard]] virtual bool isFixed() const noexcept = 0;

  /**
   * @return true if the domain contains the value, else false.
   */
  [[nodiscard]] virtual bool contains(Int value) const noexcept = 0;

  /**
   * @return true if the domain is an interval, else false.
   */
  [[nodiscard]] virtual bool isInterval() const noexcept = 0;

  virtual void fix(Int) = 0;

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
  IntervalDomain(Int lb, Int ub);

  [[nodiscard]] Int lowerBound() const override;
  [[nodiscard]] Int upperBound() const override;
  [[nodiscard]] std::pair<Int, Int> bounds() const override;
  [[nodiscard]] size_t size() const noexcept override;
  [[nodiscard]] bool isFixed() const noexcept override;
  [[nodiscard]] bool contains(Int) const noexcept override;
  [[nodiscard]] bool isInterval() const noexcept override;

  [[nodiscard]] std::vector<DomainEntry> relativeComplementIfIntersects(
      Int lb, Int ub) const override;

  void setLowerBound(Int lb);

  void setUpperBound(Int ub);

  void fix(Int value) override;

  void intersect(Int lb, Int ub);

  [[nodiscard]] bool isDisjoint(const SetDomain&) const;
  [[nodiscard]] bool isDisjoint(const IntervalDomain&) const;

  bool operator==(const IntervalDomain&) const;

  bool operator!=(const IntervalDomain&) const;
};

class SetDomain : public Domain {
 private:
  std::vector<Int> _values;

 public:
  explicit SetDomain(std::vector<Int>&&);
  explicit SetDomain(const std::vector<Int>&);

  [[nodiscard]] const std::vector<Int>& values() const;

  [[nodiscard]] Int lowerBound() const override;
  [[nodiscard]] Int upperBound() const override;
  [[nodiscard]] std::pair<Int, Int> bounds() const override;
  [[nodiscard]] size_t size() const noexcept override;
  [[nodiscard]] bool isFixed() const noexcept override;
  [[nodiscard]] bool contains(Int) const noexcept override;
  [[nodiscard]] bool isInterval() const noexcept override;

  [[nodiscard]] std::vector<DomainEntry> relativeComplementIfIntersects(
      Int lb, Int ub) const override;

  void remove(Int value);
  void remove(const std::vector<Int>& values);

  /**
   * @brief removes all values that are strictly less than the given value.
   *
   * @param val the minimum value that is allowed in the domain.
   */

  void removeBelow(Int val);

  /**
   * @brief removes all values that are strictly greater than the given value.
   *
   * @param val the maximum value that is allowed in the domain.
   */
  void removeAbove(Int val);

  /**
   * @brief removes all values in the domain, except the values in the given
   * vector.
   *
   * @param values the values that are to be kept in the domain.
   */
  void intersect(const std::vector<Int>&);

  [[nodiscard]] bool isDisjoint(const SetDomain&) const;
  [[nodiscard]] bool isDisjoint(const IntervalDomain&) const;

  void fix(Int value) override;

  bool operator==(const SetDomain&) const;

  bool operator!=(const SetDomain&) const;
};

class SearchDomain : public Domain {
 private:
  std::variant<IntervalDomain, SetDomain> _domain;

 public:
  explicit SearchDomain(std::vector<Int>&&);
  explicit SearchDomain(const std::vector<Int>&);
  explicit SearchDomain(Int lb, Int ub);

  [[nodiscard]] const std::variant<IntervalDomain, SetDomain>& innerDomain()
      const noexcept;

  [[nodiscard]] const std::vector<Int>& values();
  [[nodiscard]] Int lowerBound() const override;
  [[nodiscard]] Int upperBound() const override;
  [[nodiscard]] std::pair<Int, Int> bounds() const override;
  [[nodiscard]] size_t size() const noexcept override;
  [[nodiscard]] bool isFixed() const noexcept override;
  [[nodiscard]] bool contains(Int) const noexcept override;
  [[nodiscard]] bool isInterval() const noexcept override;

  [[nodiscard]] std::vector<DomainEntry> relativeComplementIfIntersects(
      Int lb, Int ub) const override;

  void remove(Int value);

  /**
   * @brief removes all values that are strictly less than the given value.
   *
   * @param val the minimum value that is allowed in the domain.
   */
  void removeBelow(Int val);

  /**
   * @brief removes all values that are strictly greater than the given value.
   *
   * @param val the maximum value that is allowed in the domain.
   */
  void removeAbove(Int newUpperBound);

  /**
   * @brief removes all values in the given vector from the domain.
   *
   * @param values the values to remove from the domain.
   */
  void remove(const std::vector<Int>& values);

  /**
   * @brief removes all values in the domain, except the values in the given
   * vector.
   *
   * @param values the values that are to be kept in the domain.
   */
  void intersect(const std::vector<Int>&);

  void intersect(Int lb, Int ub);

  void intersect(const SearchDomain& other);

  [[nodiscard]] bool isDisjoint(const SetDomain&) const;
  [[nodiscard]] bool isDisjoint(const IntervalDomain&) const;
  [[nodiscard]] bool isDisjoint(const SearchDomain&) const;

  void fix(Int value) override;

  bool operator==(const SearchDomain&) const;

  bool operator!=(const SearchDomain&) const;
};

}  // namespace atlantis
