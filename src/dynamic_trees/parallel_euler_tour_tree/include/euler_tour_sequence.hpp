#pragma once

#include <utility>

#include <sequence/parallel_skip_list/include/skip_list_base.hpp>
#include <utilities/include/seq.h>

namespace parallel_euler_tour_tree {

namespace _internal {

class Element : public parallel_skip_list::ElementBase<Element> {
 public:
  Element() : parallel_skip_list::ElementBase<Element>{} {}
  explicit Element(std::pair<int, int> id, size_t random_int)
    : parallel_skip_list::ElementBase<Element>{random_int}, id_{id}, values_{new int[height_]} {
    const bool isVertex{id.first == id.second};
    values_[0] = isVertex;
  }

  // If this element represents a vertex v, then id == (v, v). Otherwise if
  // this element represents a directed edge (u, v), then id == (u,v).
  std::pair<int, int> id_;
  // If this element represents edge (u, v), `twin` should point towards (v, u).
  Element* twin_{nullptr};
  // When batch splitting, we mark this as `true` for an edge that we will
  // splice out in the current round of recursion.
  bool split_mark_{false};

  // Get all vertices held in the skip list that contains this element.
  seq::sequence<int> GetVertices();

 private:
  friend class parallel_skip_list::ElementBase<Element>;
  static void DerivedInitialize() {}
  static void DerivedFinish() {}

  // Updates `values_`.
  void UpdateTopDown(int level);
  void UpdateTopDownSequential(int level);
  // Gets vertices held in child elements of this element and writes them into
  // the sequence starting at the offset. `values_` needs to be up to date.
  void GetVerticesBelow(seq::sequence<int>* s, int offset, int level) const;
  // Augment the skip list with the function "count the number of vertices in
  // this list" (i.e., `values_[0]` is 1 if element represents a vertex, else is
  // 0; `values_[i]` sums children's `values_[i - 1]`).
  // We update `values_` lazily since we only use it in `GetVertices()`.
  int* values_{nullptr};
};

}  // namespace _internal

}  // namespace parallel_euler_tour_tree
