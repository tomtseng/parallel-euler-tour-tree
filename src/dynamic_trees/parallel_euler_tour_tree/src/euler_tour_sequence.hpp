#pragma once

#include <sequence/parallel_skip_list/include/skip_list_base.hpp>

namespace parallel_euler_tour_tree {

namespace _internal {

class Element : public parallel_skip_list::ElementBase<Element> {
 public:
  Element() : parallel_skip_list::ElementBase<Element>{} {}
  explicit Element(size_t random_int)
    : parallel_skip_list::ElementBase<Element>{random_int} {}

  // If this element represents edge (u, v), `twin` should point towards (v, u).
  Element* twin_{nullptr};
  // When batch splitting, we mark this as `true` for an edge that we will
  // splice out in the current round of recursion.
  bool split_mark_{false};

 private:
  friend class parallel_skip_list::ElementBase<Element>;
  static void DerivedInitialize() {}
  static void DerivedFinish() {}
};

}  // namespace _internal

}  // namespace parallel_euler_tour_tree
