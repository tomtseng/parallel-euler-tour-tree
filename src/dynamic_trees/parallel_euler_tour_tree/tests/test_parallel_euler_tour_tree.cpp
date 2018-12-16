#include <dynamic_trees/parallel_euler_tour_tree/include/euler_tour_tree.hpp>
#include <dynamic_trees/parallel_euler_tour_tree/tests/simple_forest_connectivity.hpp>

#include <boost/functional/hash.hpp>
#include <cassert>
#include <random>
#include <utility>

#include <utilities/include/debug.hpp>
#include <utilities/include/hash_pair.hpp>

using EulerTourTree = parallel_euler_tour_tree::EulerTourTree;

constexpr int num_vertices{500};
constexpr int link_attempts_per_round{400};
constexpr int cut_ratio{3};
constexpr int num_rounds{10};

void CheckAllPairsConnectivity(
    const SimpleForestConnectivity& reference_solution,
    const EulerTourTree& ett) {
  for (int u = 0; u < num_vertices; u++) {
    for (int v = 0; v < num_vertices; v++) {
      assert(reference_solution.IsConnected(u, v) == ett.IsConnected(u, v));
    }
  }
}

int main() {
  std::mt19937 rng{};
  rng.seed(0);
  std::uniform_int_distribution<std::mt19937::result_type>
    vert_dist{0, num_vertices - 1};
  std::uniform_int_distribution<std::mt19937::result_type>
    coin{0, 1};

  SimpleForestConnectivity reference_solution{num_vertices};
  EulerTourTree ett{num_vertices};
  std::unordered_set<std::pair<int, int>, HashIntPairStruct> edges{};
  std::pair<int, int>* ett_input{
      pbbs::new_array_no_init<pair<int, int>>(num_vertices)};
  for (int i = 0; i < num_rounds; i++) {
    // Generate `link_attempts_per_round` edges randomly, keeping each one that
    // doesn't add a cycle into the forest. Then call `BatchLink` on all of
    // them.
    int input_len{0};
    for (int j = 0; j < link_attempts_per_round; j++) {
      const unsigned long u{vert_dist(rng)}, v{vert_dist(rng)};
      if (!reference_solution.IsConnected(u, v)) {
        reference_solution.Link(u, v);
        edges.emplace(u, v);
        ett_input[input_len++] = std::make_pair(u, v);
      }
    }
    ett.BatchLink(ett_input, input_len);
    CheckAllPairsConnectivity(reference_solution, ett);

    // Call `BatchCut` over each `cut_ratio`-th edge.
    input_len = 0;
    int cnt{0};
    for (auto e : edges) {
      if (++cnt % cut_ratio == 0) {
        ett_input[input_len++] = e;
      }
    }
    for (int j = 0; j < input_len; j++) {
      pair<int, int> cut{ett_input[j]};
      edges.erase(cut);
      if (coin(rng) == 1) {
        ett_input[j] = make_pair(cut.second, cut.first);
      }
      reference_solution.Cut(cut.first, cut.second);
    }
    ett.BatchCut(ett_input, input_len);
    CheckAllPairsConnectivity(reference_solution, ett);
  }
  pbbs::delete_array(ett_input, num_vertices);

  std::cout << "Test complete." << std::endl;
}
