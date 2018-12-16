#pragma once

// This is a naive augmented skip list where a batch of k joins/splits just does each
// sequentially, updating the augmented value after each join/split. Work bound
// is O(k log n) for k operations over n elements.
//
// The augmentation is hardcoded to the sum function with the value 1 assigned
// to each element. As such, `GetSum()` returns the size of the list.

#include <algorithm>

#include <sequence/skip_list/include/skip_list_base.hpp>

namespace skip_list {

class AugmentedElement : private ElementBase<AugmentedElement> {
  friend class ElementBase<AugmentedElement>;
 public:
  // See comments on [ElementBase<>]
  AugmentedElement();
  AugmentedElement(size_t random_int);

  ~AugmentedElement();

  // Get result of applying augmentation function over the whole list.
  // May run concurrently with other [GetAggregate] calls.
  int GetSum();

  // For each pair [<left, right>] in [joins], [left] must be the last node in
  // its list, and [right] must be the first node of in its list. Each [left]
  // must be unique, and each [right] must be unique.
  static void BatchJoin(
      std::pair<AugmentedElement*, AugmentedElement*>* joins, int len);

  static void BatchSplit(AugmentedElement** splits, int len);

  using ElementBase<AugmentedElement>::FindRepresentative;
  using ElementBase<AugmentedElement>::GetNextElement;
  using ElementBase<AugmentedElement>::GetPreviousElement;

 private:
  int* vals;

  void AllocateVals();

  // return left parent & sum from left parent's direct child (inclusive) to
  // input node (inclusive)
  std::pair<AugmentedElement*, int> FindLeftParentAndSum(int level);
  // return right parent & sum from input node (inclusive) to right parent's //
  // direct child (exclusive)
  std::pair<AugmentedElement*, int> FindRightParentAndSum(int level);
};

} // namespace skip_list
