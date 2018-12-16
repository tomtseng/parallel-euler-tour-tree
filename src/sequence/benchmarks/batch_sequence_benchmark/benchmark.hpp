#pragma once

#include <cassert>
#include <random>
#include <string>
#include <utility>
#include <vector>

#include <utilities/include/debug.hpp>
#include <utilities/include/gettime.h>
#include <utilities/include/parse_command_line.h>
#include <utilities/include/utils.h>

using std::string;

namespace batch_sequence_benchmark {

struct BenchmarkParameters {
  int num_elements;
  int batch_size;
  int num_iterations;
  std::string batch_type;
};

BenchmarkParameters GetBenchmarkParameters(int argc, char** argv) {
  commandLine P{argc, argv,
      "./benchmark [-n (num nodes)] [-k (batch size)] [-iters] [-batch-type]"};
  BenchmarkParameters parameters{};
  parameters.num_elements = P.getOptionIntValue("-n", 1000);
  parameters.batch_size =
      P.getOptionIntValue("-k", parameters.num_elements / 32);
  parameters.num_iterations = P.getOptionIntValue("-iters", 5);
  parameters.batch_type = string(P.getOptionValue("-batch-type", "random"));
  return parameters;
}

// Get locations at which we will join and split.
// If batch_type == 'random': pick locations uniformly at random.
// If batch_type == 'backward': pick locations going in right-to-left order
// starting from the end of the list
int* GetBatchIndices(int num_elements, int batch_size,
    int* perm, const std::string& batch_type) {
  int* batch_indices{pbbs::new_array_no_init<int>(batch_size)};
  if (batch_type == "random") {
    parallel_for (int i = 0; i < batch_size; i++) {
      batch_indices[i] = perm[i];
    }
  } else if (batch_type == "backward") {
    parallel_for (int i = 0; i < batch_size; i++) {
      batch_indices[i] = num_elements - i - 2;
    }
  } else {
    TRACE(batch_type);
    assert(0);
  }
  return batch_indices;
}

template <typename Element>
void ConstructJoinsAndSplitsFromIndices(int* indices, Element* elements,
    int len, pair<Element*, Element*>* joins, Element** splits) {
  parallel_for (int i = 0; i < len; i++) {
    int idx{indices[i]};
    joins[i] = make_pair(&elements[idx], &elements[idx + 1]);
    splits[i] = &elements[idx];
  }
}

// Pick `batch_size` many element locations according to `GetBatchIndices`.
// For `num_iterations` iterations, construct a list and then batch split and
// batch join on those locations. Report the median batch split and batch join
// times.
template <typename Element>
void RunBenchmark(Element* elements, const BenchmarkParameters& parameters) {
  const int num_elements{parameters.num_elements};
  const int batch_size{parameters.batch_size};
  const int num_iterations{parameters.num_iterations};

  typedef pair<Element*, Element*> ElementPPair;
  assert(0 <= batch_size && batch_size < num_elements);

  std::cout << "Running with " << nworkers() << " workers" << std::endl;

  int* perm{pbbs::new_array_no_init<int>(num_elements - 1)};
  for (int i = 0; i < num_elements - 1; i++) {
    perm[i] = i;
  }
  std::mt19937 generator{0};
  std::shuffle(perm, perm + num_elements - 1, generator);

  ElementPPair* construct_joins{
      pbbs::new_array_no_init<ElementPPair>(num_elements - 1)};
  Element** destruct_splits{
      pbbs::new_array_no_init<Element*>(num_elements - 1)};
  ConstructJoinsAndSplitsFromIndices(
      perm, elements, num_elements - 1, construct_joins, destruct_splits);

  int* batch_indices{GetBatchIndices(
      num_elements, batch_size, perm, parameters.batch_type)};
  ElementPPair* batch_joins{pbbs::new_array_no_init<ElementPPair>(batch_size)};
  Element** batch_splits{pbbs::new_array_no_init<Element*>(batch_size)};
  ConstructJoinsAndSplitsFromIndices(
      batch_indices, elements, batch_size, batch_joins, batch_splits);

  vector<double> split_times(num_iterations);
  vector<double> join_times(num_iterations);

  for (int j = 0; j < num_iterations; j++) {
    Element::BatchJoin(construct_joins, num_elements - 1);

    timer split_t; split_t.start();
    Element::BatchSplit(batch_splits, batch_size);
    split_times[j] = split_t.stop();

    timer join_t; join_t.start();
    Element::BatchJoin(batch_joins, batch_size);
    join_times[j] = join_t.stop();

    Element::BatchSplit(destruct_splits, num_elements - 1);
  }

  timer::report_time_no_newline("join", median(join_times));
  timer::report_time("split", median(split_times));

  pbbs::delete_array(perm, num_elements - 1);
  pbbs::delete_array(construct_joins, num_elements - 1);
  pbbs::delete_array(destruct_splits, num_elements - 1);
  pbbs::delete_array(batch_joins, batch_size);
  pbbs::delete_array(batch_splits, batch_size);
}

}  // namespace batch_sequence_benchmark
