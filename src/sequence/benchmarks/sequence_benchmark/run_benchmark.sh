#!/bin/bash -x

num_elements=100000000
batch_size=10000000
batch_sizes=(100 1000 10000 100000 1000000 10000000 99999999)
threads=(1 2 4 8 16 32 64 72 144)
iters=3

sequential_targets=(skip_list splay_tree)
bin_dir=$(git rev-parse --show-toplevel)/bin
time_dir=times
threads_times_output=${time_dir}/threads.txt
batch_times_output=${time_dir}/batch.txt

mkdir ${time_dir}
rm -i ${threads_times_output} ${batch_times_output}

echo "### n=${num_elements} k=${batch_size} iters=${iters}" >> $threads_times_output
echo "### n=${num_elements} iters=${iters}" >> $batch_times_output

cd parallel_skip_list
make -s
benchmark_bin=${bin_dir}/benchmark_sequence_parallel_skip_list
for t in ${threads[@]}
do
  CILK_NWORKERS=$t numactl -i all $benchmark_bin -n $num_elements -k $batch_size -iters $iters >> ../${threads_times_output}
done
echo "** parallel_skip_list (1) ************************" >> ../${batch_times_output}
for k in ${batch_sizes[@]}
do
  CILK_NWORKERS=1 $benchmark_bin -n $num_elements -k $k -iters $iters >> ../${batch_times_output}
done
echo "** parallel_skip_list (72h) ************************" >> ../${batch_times_output}
for k in ${batch_sizes[@]}
do
  CILK_NWORKERS=144 numactl -i all $benchmark_bin -n $num_elements -k $k -iters $iters >> ../${batch_times_output}
done
cd ..

for target in ${sequential_targets[@]}
do
  cd $target
  make -s
  echo "** $target ************************" >> ../${batch_times_output}
  for k in ${batch_sizes[@]}
  do
    ${bin_dir}/benchmark_sequence_${target} -n $num_elements -k $k -iters $iters >> ../${batch_times_output}
  done
  cd ..
done
