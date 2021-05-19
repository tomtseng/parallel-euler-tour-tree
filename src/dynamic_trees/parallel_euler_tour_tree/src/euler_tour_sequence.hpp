#pragma once

#include <utility>

#include <sequence/parallel_skip_list/include/skip_list_base.hpp>

namespace parallel_euler_tour_tree {

namespace _internal {

class Element : public parallel_skip_list::ElementBase<Element> {
 public:
  Element() : parallel_skip_list::ElementBase<Element>{} {}
  explicit Element(std::pair<int, int> id, size_t random_int)
    : parallel_skip_list::ElementBase<Element>{random_int}, id_{id} {}

  // If this element represents a vertex v, then id == (v, v). Otherwise if
  // this element represents a directed edge (u, v), then id == (u,v).
  std::pair<int, int> id_;
  // If this element represents edge (u, v), `twin` should point towards (v, u).
  Element* twin_{nullptr};
  // When batch splitting, we mark this as `true` for an edge that we will
  // splice out in the current round of recursion.
  bool split_mark_{false};

  // Get all vertices held in children elements of this skip list element and
  // write them into the input sequence. Returns the number of elements written.
  int GetVerticesBelow(seq::sequence<int>* seq);

 private:
  friend class parallel_skip_list::ElementBase<Element>;
  static void DerivedInitialize() {}
  static void DerivedFinish() {}

  // Count the number of vertices held in child elements of this element.
  int CountVerticesBelow();

  // Counts the number of vertices below this skip list element during
  // `GetVerticesBelow()`.
  int acc_{0};
};

}  // namespace _internal

}  // namespace parallel_euler_tour_tree
