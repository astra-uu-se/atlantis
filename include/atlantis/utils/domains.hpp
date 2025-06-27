#pragma once

#include <cstddef>
#include <iterator>
#include <variant>
#include <vector>

#include "atlantis/sortedUniqueVector.hpp"
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
  struct Iterator {
    using iterator_category = std::forward_iterator_tag;
    using difference_type = std::ptrdiff_t;
    using value_type = Int;
    using pointer = Int const*;
    using reference = const Int&;
    using range = std::pair<Int, Int>;
    using set = std::vector<Int> const*;

   private:
    std::variant<range, set> _data;
    Int _pos;

   public:
    explicit Iterator(Int lb, Int ub, Int pos);
    explicit Iterator(const std::vector<Int>&, size_t pos);

    const reference operator*() const;
    pointer operator->() const;

    Iterator& operator++();
    Iterator operator++(int);

    Iterator& operator--();
    Iterator operator--(int);

    Iterator& operator+=(size_t);
    Iterator& operator-=(size_t);

    Iterator operator+(size_t) const;
    Iterator operator-(size_t) const;

    bool operator==(const Iterator&) const;
    bool operator!=(const Iterator&) const;
  };

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
   * @return true if the domain contains the interval lb..ub, else false.
   */
  [[nodiscard]] virtual bool contains(Int lb, Int ub) const noexcept = 0;

  /**
   * @return true if the domain contains all the values in the vector, else
   * false.
   */
  [[nodiscard]] virtual bool contains(
      const SortedUniqueVector&) const noexcept = 0;

  /**
   * @return true if the domain is an interval, else false.
   */
  [[nodiscard]] virtual bool isInterval() const noexcept = 0;

  virtual void fix(Int) = 0;

  [[nodiscard]] virtual bool isDisjoint(Int lb, Int ub) const = 0;

  [[nodiscard]] virtual bool isDisjoint(const SortedUniqueVector&) const = 0;

  [[nodiscard]] virtual bool isContained(Int lb, Int ub) const = 0;

  [[nodiscard]] virtual bool isContained(const SortedUniqueVector&) const = 0;

  [[nodiscard]] virtual bool isEqual(Int lb, Int ub) const = 0;

  [[nodiscard]] virtual bool operator==(const SortedUniqueVector&) const = 0;

  [[nodiscard]] virtual bool operator!=(const SortedUniqueVector&) const = 0;

  [[nodiscard]] virtual Int at(size_t) const = 0;

  [[nodiscard]] virtual Int operator[](size_t) const = 0;

  [[nodiscard]] virtual Iterator begin() const = 0;

  [[nodiscard]] virtual Iterator end() const = 0;

  /**
   * @return if the domain is a superset of lb..ub,
   * then returns an empty vector,
   * otherwise returns the intersection of the domain and lb..ub.
   */
  [[nodiscard]] virtual std::vector<DomainEntry> createDomainEntries(
      Int lb, Int ub) const = 0;
};

class IntervalDomain : public Domain {
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
  [[nodiscard]] bool contains(Int lb, Int ub) const noexcept override;
  [[nodiscard]] bool contains(
      const SortedUniqueVector&) const noexcept override;
  [[nodiscard]] bool contains(const IntervalDomain&) const noexcept;
  [[nodiscard]] bool contains(const SetDomain&) const noexcept;
  [[nodiscard]] bool isInterval() const noexcept override;
  [[nodiscard]] Iterator begin() const override;
  [[nodiscard]] Iterator end() const override;
  [[nodiscard]] Int at(size_t) const override;
  [[nodiscard]] Int operator[](size_t) const override;

  [[nodiscard]] std::vector<DomainEntry> createDomainEntries(
      Int lb, Int ub) const override;

  void setLowerBound(Int lb);

  void setUpperBound(Int ub);

  void fix(Int value) override;

  void intersect(Int lb, Int ub);

  [[nodiscard]] bool isDisjoint(Int lb, Int ub) const override;
  [[nodiscard]] bool isDisjoint(const SortedUniqueVector&) const override;
  [[nodiscard]] bool isDisjoint(const SetDomain&) const;
  [[nodiscard]] bool isDisjoint(const IntervalDomain&) const;

  [[nodiscard]] bool isContained(Int lb, Int ub) const override;
  [[nodiscard]] bool isContained(const SortedUniqueVector&) const override;
  [[nodiscard]] bool isContained(const SetDomain&) const;
  [[nodiscard]] bool isContained(const IntervalDomain&) const;

  [[nodiscard]] bool isEqual(Int lb, Int ub) const override;
  [[nodiscard]] bool operator==(const SortedUniqueVector&) const override;
  [[nodiscard]] bool operator==(const IntervalDomain&) const;
  [[nodiscard]] bool operator==(const SetDomain&) const;

  [[nodiscard]] bool operator!=(const SortedUniqueVector&) const override;
  [[nodiscard]] bool operator!=(const IntervalDomain&) const;
  [[nodiscard]] bool operator!=(const SetDomain&) const;
};

class SetDomain : public Domain {
  std::vector<Int> _values;
  void intersect(const std::vector<Int>&);
  [[nodiscard]] bool contains(const std::vector<Int>&) const noexcept;
  [[nodiscard]] bool isDisjoint(const std::vector<Int>&) const;
  void remove(const std::vector<Int>&);

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
  [[nodiscard]] bool contains(Int lb, Int ub) const noexcept override;
  [[nodiscard]] bool contains(
      const SortedUniqueVector&) const noexcept override;
  [[nodiscard]] bool contains(const IntervalDomain&) const noexcept;
  [[nodiscard]] bool contains(const SetDomain&) const noexcept;
  [[nodiscard]] bool isInterval() const noexcept override;
  [[nodiscard]] Iterator begin() const override;
  [[nodiscard]] Iterator end() const override;
  [[nodiscard]] Int at(size_t) const override;
  [[nodiscard]] Int operator[](size_t) const override;

  [[nodiscard]] std::vector<DomainEntry> createDomainEntries(
      Int lb, Int ub) const override;

  void remove(Int value);
  void remove(Int lb, Int ub);
  void remove(const SortedUniqueVector&);
  void remove(const IntervalDomain&);
  void remove(const SetDomain&);

  /**
   * @brief removes all values that are strictly less than the given value.
   *
   * @param newLowerBound the minimum value that is allowed in the domain.
   */

  void removeBelow(Int newLowerBound);

  /**
   * @brief removes all values that are strictly greater than the given value.
   *
   * @param newUpperBound the maximum value that is allowed in the domain.
   */
  void removeAbove(Int newUpperBound);

  /**
   * @brief removes all values in the domain, except the values in the given
   * vector.
   */
  void intersect(const SortedUniqueVector&);
  void intersect(const SetDomain&);

  [[nodiscard]] bool isDisjoint(Int lb, Int ub) const override;
  [[nodiscard]] bool isDisjoint(const SortedUniqueVector&) const override;
  [[nodiscard]] bool isDisjoint(const SetDomain&) const;
  [[nodiscard]] bool isDisjoint(const IntervalDomain&) const;

  [[nodiscard]] bool isContained(Int lb, Int ub) const override;
  [[nodiscard]] bool isContained(const SortedUniqueVector&) const override;
  [[nodiscard]] bool isContained(const SetDomain&) const;
  [[nodiscard]] bool isContained(const IntervalDomain&) const;

  void fix(Int value) override;

  [[nodiscard]] bool isEqual(Int lb, Int ub) const override;
  [[nodiscard]] bool operator==(const SortedUniqueVector&) const override;
  [[nodiscard]] bool operator==(const IntervalDomain&) const;
  [[nodiscard]] bool operator==(const SetDomain&) const;

  [[nodiscard]] bool operator!=(const SortedUniqueVector&) const override;
  [[nodiscard]] bool operator!=(const IntervalDomain&) const;
  [[nodiscard]] bool operator!=(const SetDomain&) const;
};

class SearchDomain : public Domain {
  std::variant<IntervalDomain, SetDomain> _domain;
  void intersect(const std::vector<Int>&);
  void remove(const std::vector<Int>&);

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
  [[nodiscard]] bool contains(Int lb, Int ub) const noexcept override;
  [[nodiscard]] bool contains(
      const SortedUniqueVector&) const noexcept override;
  [[nodiscard]] bool contains(const IntervalDomain&) const noexcept;
  [[nodiscard]] bool contains(const SetDomain&) const noexcept;
  [[nodiscard]] bool contains(const SearchDomain&) const noexcept;
  [[nodiscard]] bool isInterval() const noexcept override;
  [[nodiscard]] Iterator begin() const override;
  [[nodiscard]] Iterator end() const override;
  [[nodiscard]] Int at(size_t) const override;
  [[nodiscard]] Int operator[](size_t) const override;

  [[nodiscard]] std::vector<DomainEntry> createDomainEntries(
      Int lb, Int ub) const override;

  /**
   * @brief removes all values that are strictly less than the given value.
   *
   * @param newLowerBound the minimum value that is allowed in the domain.
   */
  void removeBelow(Int newLowerBound);

  /**
   * @brief removes all values that are strictly greater than the given value.
   *
   * @param newUpperBound the maximum value that is allowed in the domain.
   */
  void removeAbove(Int newUpperBound);

  void remove(Int value);
  /**
   * Removes all values in the given interval from the domain.
   * @param lb the lower bound of the interval
   * @param ub the upper bound of the interval
   */
  void remove(Int lb, Int ub);
  /**
   * @brief removes all values in the given vector from the domain.
   *
   * @param vals the values to remove from the domain.
   */
  void remove(const SortedUniqueVector& vals);
  void remove(const IntervalDomain&);
  void remove(const SetDomain&);
  void remove(const SearchDomain&);

  /**
   * @brief removes all values in the domain, except the values in the given
   * vector.
   */
  void intersect(const SortedUniqueVector&);
  void intersect(Int lb, Int ub);
  void intersect(const SetDomain& other);
  void intersect(const SearchDomain& other);

  [[nodiscard]] bool isDisjoint(Int lb, Int ub) const override;
  [[nodiscard]] bool isDisjoint(const SortedUniqueVector&) const override;
  [[nodiscard]] bool isDisjoint(const SetDomain&) const;
  [[nodiscard]] bool isDisjoint(const IntervalDomain&) const;
  [[nodiscard]] bool isDisjoint(const SearchDomain&) const;

  [[nodiscard]] bool isContained(Int lb, Int ub) const override;
  [[nodiscard]] bool isContained(const SortedUniqueVector&) const override;
  [[nodiscard]] bool isContained(const SetDomain&) const;
  [[nodiscard]] bool isContained(const IntervalDomain&) const;
  [[nodiscard]] bool isContained(const SearchDomain&) const;

  void fix(Int value) override;

  [[nodiscard]] bool isEqual(Int lb, Int ub) const override;
  [[nodiscard]] bool operator==(const SortedUniqueVector&) const override;
  [[nodiscard]] bool operator==(const IntervalDomain&) const;
  [[nodiscard]] bool operator==(const SetDomain&) const;
  [[nodiscard]] bool operator==(const SearchDomain&) const;

  [[nodiscard]] bool operator!=(const SortedUniqueVector&) const override;
  [[nodiscard]] bool operator!=(const IntervalDomain&) const;
  [[nodiscard]] bool operator!=(const SetDomain&) const;
  [[nodiscard]] bool operator!=(const SearchDomain&) const;
};

}  // namespace atlantis
