#include <dynamic_trees/parallel_euler_tour_tree/tests/simple_forest_connectivity.hpp>

#include <algorithm>
#include <cassert>
#include <limits>
#include <stack>

SimpleForestConnectivity::SimpleForestConnectivity(int num_vertices)
    : num_vertices_{num_vertices} {
  component_ids_ = std::vector<int>(num_vertices);
  for (int i = 0; i < num_vertices; i++) {
    component_ids_[i] = i;
  }
  adjacency_list_ = std::vector<std::unordered_set<int>>(num_vertices);
  visited_ = std::vector<bool>(num_vertices);
}

int SimpleForestConnectivity::MinimumVertexInComponent(int v) {
  fill(visited_.begin(), visited_.end(), false);
  int min_label{std::numeric_limits<int>::max()};
  std::stack<int> s{};
  s.push(v);
  while (!s.empty()) {
    int curr{s.top()};
    s.pop();
    if (!visited_[curr]) {
      visited_[curr] = true;
      min_label = std::min(min_label, curr);
      for (auto u : adjacency_list_[curr]) {
        s.push(u);
      }
    }
  }
  return min_label;
}

void SimpleForestConnectivity::AssignIdToComponent(int v, int id) {
  // Assign component_ids_[u] = id for all u in the same connected component as
  // `v`
  fill(visited_.begin(), visited_.end(), false);
  std::stack<int> s;
  s.push(v);
  while (!s.empty()) {
    int curr{s.top()};
    s.pop();
    if (!visited_[curr]) {
      visited_[curr] = true;
      component_ids_[curr] = id;
      for (auto u : adjacency_list_[curr]) {
        s.push(u);
      }
    }
  }
}

void SimpleForestConnectivity::Link(int u, int v) {
  assert(!IsConnected(u, v));
  if (component_ids_[u] < component_ids_[v]) {
    AssignIdToComponent(v, component_ids_[u]);
  } else {
    AssignIdToComponent(u, component_ids_[v]);
  }
  adjacency_list_[u].insert(v);
  adjacency_list_[v].insert(u);
}

void SimpleForestConnectivity::Cut(int u, int v) {
  assert(adjacency_list_[u].find(v) != adjacency_list_[u].end());
  adjacency_list_[u].erase(v);
  adjacency_list_[v].erase(u);
  AssignIdToComponent(u, MinimumVertexInComponent(u));
  AssignIdToComponent(v, MinimumVertexInComponent(v));
}

bool SimpleForestConnectivity::IsConnected(int u, int v) const {
  return component_ids_[u] == component_ids_[v];
}
