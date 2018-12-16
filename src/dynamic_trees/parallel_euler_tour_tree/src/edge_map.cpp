#include <dynamic_trees/parallel_euler_tour_tree/src/edge_map.hpp>

#include <utility>

namespace parallel_euler_tour_tree {

namespace _internal {

EdgeMap::EdgeMap(int num_vertices)
    : map_{nullptr, static_cast<size_t>(num_vertices - 1),
          std::make_pair(-1, -1), std::make_pair(-2, -2)} {}

EdgeMap::~EdgeMap() {
  map_.del();
}

bool EdgeMap::Insert(int u, int v, Element* edge) {
  if (u > v) {
    std::swap(u, v);
    edge = edge->twin_;
  }
  return map_.insert(make_pair(u, v), edge);
}

bool EdgeMap::Delete(int u, int v) {
  if (u > v) {
    std::swap(u, v);
  }
  return map_.deleteVal(make_pair(u, v));
}

Element* EdgeMap::Find(int u, int v) {
  if (u > v) {
    Element* vu{*map_.find(make_pair(v, u))};
    return vu == nullptr ? nullptr : vu->twin_;
  } else {
    return *map_.find(make_pair(u, v));
  }
}

void EdgeMap::FreeElements(list_allocator<Element>* allocator) {
  parallel_for (size_t i = 0; i < map_.capacity; i++) {
    auto kv{map_.table[i]};
    auto key{get<0>(kv)};
    if (key != map_.empty_key && key != map_.tombstone) {
      Element* element{get<1>(kv)};
      element->twin_->~Element();
      allocator->free(element->twin_);
      element->~Element();
      allocator->free(element);
    }
  }
}

}  // namespace _internal

}  // namespace parallel_euler_tour_tree
