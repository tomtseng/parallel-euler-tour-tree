This benchmark measures the performance of batch links and batch cuts on
dynamic trees data structures.

This was written to run some one-off experiments, so it was not properly
designed to be easy to use, easy to modify, or easy to interpret.

### How do I run it?

Primarily this is run through running `./generate_graphs.py` to generate graph
files and then running benchmarks on the graphs through `./run_benchmark.sh`.
The constants at the top of the script files may be tweaked. This benchmarks
some dynamic trees implementations on a few graphs with varying numbers of
threads and varying batch sizes. Timings are output to the `times/` directory.

In `./run_benchmark.sh`, the single-threaded timings are all run in parallel. If
your machine doesn't have enough resources to handle so many concurrent runs,
the timings will be inaccurate.

To run things manually, some code for generating graphs is given in `data/src/`.
The benchmark code expects that graph files should be given in the [adjacency
graph format from the Problem Based Benchmark
Suite](http://www.cs.cmu.edu/~pbbs/benchmarks/graphIO.html). The existing graph
generators are run like
```
<base code directory>/bin/generate_path_graph <num vertices> <output_file_path>
```
Then we can go into one of the implementation directories, `make` the benchmark,
and run
```
<base code directory>/bin/benchmark_dynamic_trees_<implementation> -iters <number of iterations> <input_graph_file_path>
```

### What does it time?

Take the list of edges in the input graph and shuffle it randomly.  For various
batch sizes k, for some number of iterations, construct the full graph, batch
split on the first k edges, and batch join the k edges back in.  Output the
median time to perform the batch split and join for that batch size.
