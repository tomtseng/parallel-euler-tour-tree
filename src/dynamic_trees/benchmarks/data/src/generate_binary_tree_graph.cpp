#include <vector>

#include <dynamic_trees/benchmarks/data/src/graph_io.hpp>
#include <utilities/include/parse_command_line.h>

typedef long long ll;
using std::vector;

// Generates a complete binary tree.
int main(int argc, char* argv[]) {
  commandLine P{argc, argv, "n <outFile>"};
  pair<ll,char*> in{P.sizeAndFileName()};
  ll n{in.first};
  char* filename{in.second};

  vector<vector<ll>> adj_list(n);
  for (ll i = 0; i < n; i++) {
    if (2 * i + 1 < n) {
      adj_list[i].push_back(2 * i + 1);
      adj_list[2 * i + 1].push_back(i);
    }
    if (2 * i + 2 < n) {
      adj_list[i].push_back(2 * i + 2);
      adj_list[2 * i + 2].push_back(i);
    }
  }

  PrintAdjGraph(adj_list, string(filename));
  return 0;
}
