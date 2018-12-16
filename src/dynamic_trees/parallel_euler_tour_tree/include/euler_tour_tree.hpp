#pragma once

#include <utility>

#include <dynamic_trees/parallel_euler_tour_tree/src/edge_map.hpp>
#include <dynamic_trees/parallel_euler_tour_tree/src/euler_tour_sequence.hpp>
#include <utilities/include/random.h>

namespace parallel_euler_tour_tree {

// Euler tour trees represent forests. We may add an edge using `Link`, remove
// an edge using `Cut`, and query whether two vertices are in the same tree
// using `IsConnected`. This implementation can also exploit parallelism when
// many edges are added at once through `BatchLink` or many edges are deleted at
// once through `BatchCut`.
class EulerTourTree {
 public:
  EulerTourTree() = delete;
  // Initializes n-vertex forest with no edges.
  explicit EulerTourTree(int num_vertices);
  ~EulerTourTree();
  EulerTourTree(const EulerTourTree&) = delete;
  EulerTourTree(EulerTourTree&&) = delete;
  EulerTourTree& operator=(const EulerTourTree&) = delete;
  EulerTourTree& operator=(EulerTourTree&&) = delete;

  // Returns true if `u` and `v` are in the same tree in the represented forest.
  bool IsConnected(int u, int v) const;
  // Adds edge {`u`, `v`} to forest. The addition of this edge must not create a
  // cycle in the graph.
  void Link(int u, int v);
  // Removes edge {`u`, `v`} from forest. The edge must be present in the
  // forest.
  void Cut(int u, int v);

  // Adds all edges in the `len`-length array `links` to the forest. Adding
  // these edges must not create cycles in the graph.
  void BatchLink(std::pair<int, int>* links, int len);
  // Removes all edges in the `len`-length array `cuts` from the forest. These
  // edges must be present in the forest and must be distinct.
  void BatchCut(std::pair<int, int>* cuts, int len);

 private:
  void BatchCutRecurse(std::pair<int, int>* cuts, int len,
      bool* ignored, _internal::Element** join_targets,
      _internal::Element** edge_elements);

  int num_vertices_;
  _internal::Element* vertices_;
  _internal::EdgeMap edges_;
  pbbs::random randomness_;
};

}  // namespace parallel_euler_tour_tree
