#pragma once

#include <utility>

#include <sequence/parallel_skip_list/include/skip_list_base.hpp>

namespace parallel_skip_list {

// Batch-parallel augmented skip list. Currently, the augmentation is
// hardcoded to the sum function with the value 1 assigned to each element. As
// such, `GetSum()` returns the size of the list.
//
// TODO(tomtseng): Allow user to query for the sum between two elements in a
// list.
// TODO(tomtseng): Allow user to pass in their own arbitrary associative augmentation
// functions. The contract for `GetSum` on a cyclic list should be that the
// function will be applied starting from `this`, because where we begin
// applying the function could matter for non-commutative functions.
class AugmentedElement : private ElementBase<AugmentedElement> {
  friend class ElementBase<AugmentedElement>;
 public:
  using ElementBase<AugmentedElement>::Initialize;
  using ElementBase<AugmentedElement>::Finish;

  // See comments on `ElementBase<>`.
  AugmentedElement();
  explicit AugmentedElement(size_t random_int);
  ~AugmentedElement();

  // Get result of applying augmentation function over the whole list.
  int GetSum() const;

  // For each `{left, right}` in the `len`-length array `joins`, concatenate the
  // list that `left` lives in to the list that `right` lives in.
  //
  // `left` must be the last node in its list, and `right` must be the first
  // node of in its list. Each `left` must be unique, and each `right` must be
  // unique.
  static void BatchJoin(
      std::pair<AugmentedElement*, AugmentedElement*>* joins, int len);

  // For each `v` in the `len`-length array `splits`, split `v`'s list right
  // after `v`.
  static void BatchSplit(AugmentedElement** splits, int len);

  // For each `i`=0,1,...,`len`-1, assign value `new_values[i]` to element
  // `elements[i]`.
  static void BatchUpdate(
      AugmentedElement** elements, int* new_values, int len);

  using ElementBase<AugmentedElement>::FindRepresentative;
  using ElementBase<AugmentedElement>::GetPreviousElement;
  using ElementBase<AugmentedElement>::GetNextElement;

 private:
  static void DerivedInitialize();
  static void DerivedFinish();

  // Update aggregate value of node and clear `join_update_level` after joins.
  void UpdateTopDown(int level);
  void UpdateTopDownSequential(int level);

  int* values_;
  // When updating augmented values, this marks the lowest index at which the
  // `values_` needs to be updated.
  int update_level_;
};

}  // namespace parallel_skip_list
