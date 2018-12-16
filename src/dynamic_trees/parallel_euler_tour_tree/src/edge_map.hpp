#pragma once

#include <utility>

#include <utilities/include/concurrentMap.h>
#include <utilities/include/hash_pair.hpp>
#include <utilities/include/list_allocator.h>
#include <dynamic_trees/parallel_euler_tour_tree/src/euler_tour_sequence.hpp>

namespace parallel_euler_tour_tree {

namespace _internal {

// Used in Euler tour tree for mapping directed edges (pairs of ints) to the
// sequence element in the Euler tour representing the edge.
//
// Only one of (u, v) and (v, u) should be added to the map; we can find the
// other edge using the `twin_` pointer in `Element`.
class EdgeMap {
 public:
  EdgeMap() = delete;
  explicit EdgeMap(int num_vertices);
  ~EdgeMap();

  bool Insert(int u, int v, Element* edge);
  bool Delete(int u, int v);
  Element* Find(int u, int v);

  // Deallocate all elements held in the map. This assumes that all elements
  // in the map were allocated through `allocator`.
  void FreeElements(list_allocator<Element>* allocator);

 private:
  concurrent_map::concurrentHT<
      std::pair<int, int>, Element*, HashIntPairStruct> map_;
};

}  // namespace _internal

}  // namespace parallel_euler_tour_tree
