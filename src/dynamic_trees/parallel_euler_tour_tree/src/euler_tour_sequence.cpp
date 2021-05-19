#include <dynamic_trees/parallel_euler_tour_tree/include/euler_tour_sequence.hpp>

namespace parallel_euler_tour_tree {

namespace _internal {

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
      (*s)[offset] = id_.first;
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
      cilk_spawn curr->GetVerticesBelow(&vertices, offset, level);
      offset += curr->values_[level];
      curr = curr->neighbors_[level].next;
    } while (curr != nullptr && curr != top_element);
    cilk_sync;
  }
  return vertices;
}

}  // namespace _internal

}  // namespace parallel_euler_tour_tree
