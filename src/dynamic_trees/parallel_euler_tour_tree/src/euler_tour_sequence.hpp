#pragma once

#include <utility>

#include <sequence/parallel_skip_list/include/skip_list_base.hpp>

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

void Element::UpdateTopDownSequential(int level) {
  if (level == 0) {
    // values_[0] filled in by constructor
    return;
  }
  UpdateTopDownSequential(level - 1);

  int sum{values_[level - 1]};
  Element* curr{neighbors_[level - 1].next};
  while (curr != nullptr && curr->height_ < level + 1) {
    curr->UpdateTopDownSequential(level - 1);
    sum += curr->values_[level - 1];
    curr = curr->neighbors_[level - 1].next;
  }
  values_[level] = sum;
}

void Element::UpdateTopDown(int level) {
  if (level <= 6) {
    // It's not worth spawning a bunch of threads once we get close to the bottom of
    // the list and are not doing as much work per thread. Instead just run
    // sequentially.
    return UpdateTopDownSequential(level);
  }

  // Recursively update augmented values of children.
  Element* curr{this};
  do {
    cilk_spawn curr->UpdateTopDown(level - 1);
    curr = curr->neighbors_[level - 1].next;
  } while (curr != nullptr && curr->height_ < level + 1);
  cilk_sync;

  // Now that children have correct augmented values, update own augmented
  // value.
  int sum{values_[level - 1]};
  curr = neighbors_[level - 1].next;
  while (curr->height_ < level + 1) {
    sum += curr->values_[level - 1];
    curr = curr->neighbors_[level - 1].next;
  }
  values_[level] = sum;
}

void Element::GetVerticesBelow(seq::sequence<int>* s, int offset, int level) const {
  if (level == 0) {
    if (values_[0]) {
      s[offset] = id_.first;
    }
    return;
  }
  const Element* curr{this};
  if (level <= 6) {
    // run sequentially once we're near the bottom of the list and not doing as
    // much work per thread
    do {
      curr->GetVerticesBelow(s, offset, level - 1);
      offset += curr->values_[level - 1];
      curr = curr->neighbors_[level - 1].next;
    } while (curr != nullptr && curr->height_ < level + 1);
  } else {  // run in parallel
    do {
      cilk_spawn curr->GetVerticesBelow(s, offset, level - 1);
      offset += curr->values_[level - 1];
      curr = curr->neighbors_[level - 1].next;
    } while (curr != nullptr && curr->height_ < level + 1);
    cilk_sync;
  }
}

seq::sequence<int> Element::GetVertices() {
  // get element at the top level of the list
  Element* const top_element{FindRepresentative()};
  const int level = top_element->height_ - 1;

  // fill in `values_` for all elements in list
  {
    Element* curr{top_element};
    do {
      cilk_spawn curr->UpdateTopDown(level);
      curr = curr->neighbors_[level].next;
    } while (curr != nullptr && curr != top_element);
    cilk_sync;
  }

  size_t num_vertices{0};
  {
    Element* curr = top_element;
    do {
      num_vertices += curr->values_[level];
      curr = curr->neighbors_[level].next;
    } while (curr != nullptr && curr != top_element);
  }

  seq::sequence<int> vertices{num_vertices};
  {
    int offset{0};
    Element* curr = top_element;
    do {
      cilk_spawn curr->GetVerticesBelow(&vertices, level, offset);
      offset += curr->values_[level];
      curr = curr->neighbors_[level].next;
    } while (curr != nullptr && curr != top_element);
  }
  return vertices;
}

}  // namespace _internal

}  // namespace parallel_euler_tour_tree
