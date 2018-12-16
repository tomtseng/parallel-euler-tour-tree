#pragma once

#include <cassert>
#include <fstream>
#include <random>
#include <string>
#include <utility>
#include <vector>

#include <utilities/include/gettime.h>
#include <utilities/include/parse_command_line.h>
#include <utilities/include/utils.h>

namespace dynamic_trees_benchmark {

struct ReadGraphOutput {
  int num_vertices;
  int num_edges;
  std::pair<int, int>* edges;
};

// Returns a list of edges. Stores number of nodes and number of edges in `*np`
// and `*mp`.
// Randomly flips edge directions.
ReadGraphOutput ReadGraph(char* graph_filename) {
  std::ifstream infile;
  infile.open(graph_filename);
  struct ReadGraphOutput output;

  string input_type;
  infile >> input_type;
  assert(input_type == "AdjacencyGraph");
  infile >> output.num_vertices >> output.num_edges;
  output.num_edges /= 2;  // symmetric graph
  const int n{output.num_vertices}, m{output.num_edges};
  output.edges = pbbs::new_array_no_init<pair<int, int>>(m);

  int* offsets{pbbs::new_array_no_init<int>(n)};
  for (int i = 0; i < n; i++) {
    infile >> offsets[i];
  }

  std::uniform_int_distribution<int> coin{0, 1};
  std::mt19937 generator{0};
  int curr_vert{0};
  int edges_index{0};
  for (int i = 0; i < 2 * m; i++) {
    while (curr_vert < n - 1 && offsets[curr_vert + 1] == i) {
      curr_vert++;
    }
    int v;
    infile >> v;
    if (curr_vert < v) {  // don't want to read both directions of edge
      // randomly swap direction of edge
      if (coin(generator)) {
        output.edges[edges_index++] = std::make_pair(curr_vert, v);
      } else {
        output.edges[edges_index++] = std::make_pair(v, curr_vert);
      }
    }
  }

  pbbs::delete_array(offsets, n);
  infile.close();
  return output;
}

// For `num_iters` iterations, construct a forest from the `m` edges in `edges`,
// then batch cut and batch link the first `batch_size` edges in `edges`.
// Report the median batch cut and batch link time.
template <typename Forest>
void UpdateForest(Forest* forest, std::pair<int, int>* edges,
    int batch_size, int num_iters, int m) {
  vector<double> cut_times(num_iters);
  vector<double> link_times(num_iters);

  for (int j = 0; j < num_iters; j++) {
    if (m == batch_size) {
      // this if statement isn't necessary, we could just only use the else
      // clause. this is just to ignore unnecessary construction/deconstruction

      timer link_t; link_t.start();
      forest->BatchLink(edges, batch_size);
      link_times[j] = link_t.stop();

      timer cut_t; cut_t.start();
      forest->BatchCut(edges, batch_size);
      cut_times[j] = cut_t.stop();
    } else {
      forest->BatchLink(edges, m);  // construct

      timer cut_t; cut_t.start();
      forest->BatchCut(edges, batch_size);
      cut_times[j] = cut_t.stop();

      timer link_t; link_t.start();
      forest->BatchLink(edges, batch_size);
      link_times[j] = link_t.stop();

      forest->BatchCut(edges, m);  // destruct
    }
  }
  const string batch_str{to_string(batch_size)};
  timer::report_time_no_newline("link-" + batch_str, median(link_times));
  timer::report_time("cut-" + batch_str, median(cut_times));
}

template <typename Forest>
void RunBenchmark(int argc, char** argv) {
  commandLine P{argc, argv, "[-iters] graph_filename"};
  int num_iters{P.getOptionIntValue("-iters", 4)};
  char* graph_filename{P.getArgument(0)};

  std::cout << "Running with " << nworkers() << " workers" << std::endl;
  ReadGraphOutput graph_info{ReadGraph(graph_filename)};
  const int m{graph_info.num_edges};
  std::pair<int, int>* edges{graph_info.edges};
  std::mt19937 generator{0};
  std::shuffle(edges, edges + m, generator);

  Forest forest{graph_info.num_vertices};

  for (int batch_size = 100; batch_size < m; batch_size *= 10) {
    UpdateForest(&forest, edges, batch_size, num_iters, m);
  }
  UpdateForest(&forest, edges, m, num_iters, m);

  pbbs::delete_array(edges, m);
}

}  // namespace dynamic_trees_benchmark
