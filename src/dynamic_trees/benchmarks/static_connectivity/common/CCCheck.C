// This code is part of the paper "A Simple and Practical Linear-Work
// Parallel Algorithm for Connectivity" in Proceedings of the ACM
// Symposium on Parallelism in Algorithms and Architectures (SPAA),
// 2014.  Copyright (c) 2014 Julian Shun, Laxman Dhulipala and Guy
// Blelloch
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the
// "Software"), to deal in the Software without restriction, including
// without limitation the rights (to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Software, and to
// permit persons to whom the Software is furnished to do so, subject to
// the following conditions:
//
// The above copyright notice and this permission notice shall be included
// in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
// LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
// OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
// WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

#include <iostream>
#include <algorithm>
#include <cstring>
#include "parallel.h"
#include "IO.h"
#include "graph.h"
#include "graphIO.h"
#include "parseCommandLine.h"
using namespace std;
using namespace benchIO;

void BFS(graph<intT> GA, intT start, intT* labels, bool* Visited, intT* Frontier) {
  intT numVertices = GA.n;
  intT numEdges = GA.m;
  vertex<intT> *G = GA.V;
  intT componentID = labels[start];
  Visited[start] = 1;
  Frontier[0] = start;
  intT bot = 0;
  intT top = 1;

  while (top > bot) {
    intT v = Frontier[bot++];
    for (intT j=0; j < G[v].degree; j++) {
      intT ngh = G[v].Neighbors[j];
      if(labels[ngh] != componentID) {
	cout << "vertex "<<ngh<<" has label "<<labels[ngh]<<
	  " but should have label "<<componentID<<endl;
	abort();
      }
      if (!Visited[ngh]) {
	Frontier[top++] = ngh;
	Visited[ngh] = 1;
      }
    }
  }
}


void check(graph<intT> G, intT* labels) {
  intT n = G.n;
  bool* Visited = newA(bool,n);
  parallel_for(intT i=0;i<n;i++) Visited[i] = 0;
  bool* Used = newA(bool,n);
  parallel_for(intT i=0;i<n;i++) Used[i] = 0;
  intT* Frontier = newA(intT,n);

  for(intT i=0;i<n;i++) {
    if(!Visited[i]) {
      if(!Used[labels[i]]) Used[labels[i]] = 1;
      else { cout << "Different components have same label\n"; abort(); }
      BFS(G,i,labels,Visited,Frontier);
    }
  }
  free(Frontier);
  free(Used);
  free(Visited);
}


int parallel_main(int argc, char* argv[]) {
  commandLine P(argc,argv,"<inFile> <outfile>");
  pair<char*,char*> fnames = P.IOFileNames();
  char* iFile = fnames.first;
  char* oFile = fnames.second;
  graph<intT> G = readGraphFromFile<intT>(iFile);
  _seq<intT> Output = readIntArrayFromFile<intT>(oFile);

  if(G.n != Output.n) { cout << "Number of vertices don't match\n"; abort(); }

  check(G,Output.A);
  G.del();
  Output.del();
  cout << "Labeling is correct\n";
}
