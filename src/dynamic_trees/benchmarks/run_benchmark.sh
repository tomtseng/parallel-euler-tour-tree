#!/bin/bash -x

n=10000000
threads=(2 4 8 16 32 64 72 144)
iters=3
graphs=('binary_tree' 'star' 'path' 'recursive_tree')

sequential_targets=('link_cut_tree' 'skip_list_ett' 'splay_tree_ett')
bin_dir=$(git rev-parse --show-toplevel)/bin
graphs_dir='data/graphs'
output_dir='times'

get_graph_file () {
  graph_file='../'${graphs_dir}'/'$1'_'$n
}

get_output_file () {
  output_file='../'${output_dir}'/'$1'-'$2'.txt'
}

get_output_file () {
  output_file='../'${output_dir}'/'$1'-'$2'.txt'
}

num_processes=0
declare -a process_ids

save_last_process_id () {
  process_ids[${num_processes}]=$!
  num_processes+=1
}

wait_for_processes () {
  for id in ${process_ids[*]}; do
    wait $id
  done
}

#*******************

mkdir $output_dir
rm -i $output_dir/*

cd parallel_ett
make -s
benchmark_bin=${bin_dir}/benchmark_dynamic_trees_parallel_ett
for g in ${graphs[@]}
do
  get_graph_file $g
  get_output_file 'parallel_ett' $g
  for t in ${threads[@]}
  do
    CILK_NWORKERS=$t numactl -i all $benchmark_bin -iters $iters $graph_file >> $output_file
  done
done

for g in ${graphs[@]}
do
  get_graph_file $g
  get_output_file 'parallel_ett' $g
  CILK_NWORKERS=1 $benchmark_bin -iters $iters $graph_file >> $output_file &
  save_last_process_id
done
cd ..

for target in ${sequential_targets[@]}
do
  cd $target
  make -s
  for g in ${graphs[@]}
  do
    get_graph_file $g
    get_output_file $target $g
    ${bin_dir}/benchmark_dynamic_trees_${target} -iters $iters $graph_file >> $output_file &
    save_last_process_id
  done
  cd ..
done

wait_for_processes
