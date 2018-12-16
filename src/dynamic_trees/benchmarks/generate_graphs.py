#!/usr/bin/env python
import subprocess

num_vertices = [10000000]
graphs = ["binary_tree", "path", "recursive_tree", "star"]
output_directory = "data/graphs/"

subprocess.check_call(["make"])
bin_directory = \
    subprocess.check_output(['git', 'rev-parse', '--show-toplevel'])[:-1]  \
        + '/bin'

for n in num_vertices:
    for graph in graphs:
        binary_name = bin_directory + "/generate_" + graph + "_graph"
        output_name = output_directory + graph + "_" + str(n)
        print binary_name, output_name
        subprocess.check_call([binary_name, str(n), output_name])
        print "generated " + graph + " " + str(n)
