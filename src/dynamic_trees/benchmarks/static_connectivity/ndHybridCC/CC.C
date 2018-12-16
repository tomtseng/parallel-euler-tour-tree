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

#include "utils.h"
#include "sequence.h"
#include "graph.h"
#include "parallel.h"
#include "gettime.h"
#include <limits.h>
#include <stdlib.h>
#include <cmath>
#include "graphUtils.h"
#include "edgeHash.h"
#include "randPerm.C"

using namespace std;

#define TODENSE 5
#define TOSPARSE 5

//for debugging
void printGraph(graph<intT> G) {
  for(intT i=0;i<G.n;i++) {
    cout<<i<<": ";
    for(intT j=0;j<G.V[i].degree;j++) 
      cout<<G.V[i].Neighbors[j]<<" ";
    cout<<endl;
  }
}

timer t0,t1,t2,t3,t4,t5,t6,t7,t8,t9,t10,t11;

void clearAll() {
  t0.clear();
  t1.clear();
  t2.clear();
  t3.clear();
  t4.clear();
  t5.clear();
  t6.clear();
  t7.clear();
  t8.clear();
  t9.clear();
  t10.clear();
  t11.clear();
}

void reportAll() {
  t0.reportTotal("randomPerm");
  t1.reportTotal("init");
  t2.reportTotal("add new bfs centers");
  t3.reportTotal("bfs preprocess phase");
  t4.reportTotal("bfs sparse");
  t5.reportTotal("bfs dense");
  t6.reportTotal("compute new IDS");
  t11.reportTotal("remove intra-component edges");
  t7.reportTotal("create edge pairs");
  t9.reportTotal("contract graph - remdups");
  t10.reportTotal("contract graph - relabel + remove singletons + collect");
  t8.reportTotal("relabel non-singletons after recursion");
}

typedef pair<intT, intT> intPair;

struct nonNegF{bool operator() (intT a) {return (a>=0);}};
struct cmpFirst { bool operator() (intPair a, intPair b) { return a.first < b.first; }};
struct nonSelfEdge{bool operator() (edge<intT> a) {return (a.u != a.v); }};

/**
  Generates a integer sequence of shifts, where shifts[i] is the start
  of the vertices to take on round i.
 **/
uintT *generateShifts(intT n, double beta, vertex<intT> *G) {
  //only need (ln n)/beta levels
  uintT maxLevel = min<uintT>(n+1,2+ceil(log(n)/beta));
  uintT *betas = newA(uintT, maxLevel);
  {parallel_for(intT i=0; i<maxLevel; i++) { 
    betas[i] =floor(exp(i*beta));
  }}
  uintT last = sequence::plusScan(betas, betas, maxLevel);
  return betas;
}

//Returns an array of labels. Vertices in the same component will have
//the same label
intT* genDecomposition(graph<intT>& GA, double beta, intT decompositionRound) {
    intT n = GA.n;
    intT m = GA.m;
    vertex<intT> *G = GA.V;

    t0.start();
    intT *vertexPerm;
    if (decompositionRound) { //only generate vertexPerm if after first round
      vertexPerm = newA(intT, GA.n);
      {parallel_for(intT i=0; i<GA.n; i++) { vertexPerm[i] = i;}}
      randPerm<intT>(vertexPerm, GA.n);
    }
    t0.stop();

    t1.start();
    // generate intervals of vertices to add to BFS 
    uintT *shifts = generateShifts(n, beta, G);  

    intT* componentID = newA(intT,n);
    parallel_for(intT i=0;i<n;i++) {
      componentID[i] = INT_T_MAX;
    }

    intT* Frontier = newA(intT,n);
    intT *FrontierNext = newA(intT, m+1); // Used to filter out the new frontier. 
    intT* Degrees = newA(intT,n+1);
    intT* flags = newA(intT,n+1);

    bool* fron = newA(bool,n);
    bool* fronNext = newA(bool,n);

    typedef pair<intT,vertex<intT> > intVertexPair;
    intVertexPair* frontierVertices = newA(intVertexPair,n);

    intT fSize = 0;
    intT totalVisited = 0;
    intT round = 0;
    t1.stop();
    int dense = 0;
    //BFS
    while(fSize > 0 || totalVisited < n) {

      t2.start();

      //get new start vertices
      intT start = shifts[round];
      intT end = min<uintT>(shifts[round+1],n);
      intT size = end-start;

      //add vertex only if it has not yet been visited
      if(decompositionRound)
	parallel_for(intT i=0;i<size;i++) flags[i] = (componentID[vertexPerm[i+start]] == INT_T_MAX);
      else
	parallel_for(intT i=0;i<size;i++) flags[i] = (componentID[i+start] == INT_T_MAX);
      flags[size] = 0;
      intT numAdded = sequence::plusScan(flags,flags,size+1);

      if(decompositionRound) //after first round, use vertexPerm
	parallel_for(intT i=0;i<size;i++) {
	  intT offset = flags[i];
	  if(flags[i+1] != offset) {
	    intT v = vertexPerm[start+i];
	    if(dense) fron[v] = 1; else Frontier[fSize+offset] = v;
	    componentID[v] = v;
	  }
	}
      else //first round, don't use vertexPerm
	parallel_for(intT i=0;i<size;i++) {
	  intT offset = flags[i];
	  if(flags[i+1] != offset) {
	    intT v = start+i;
	    if(dense) fron[v] = 1; else Frontier[fSize+offset] = v;
	    componentID[v] = v;
	  }
	}
      fSize += numAdded;
      totalVisited += numAdded;
      t2.stop();

      if(!dense && fSize*TODENSE > n) {
	//switch to dense
	dense = 1;
	parallel_for(intT i=0;i<n;i++) fron[i] = 0;
	parallel_for(intT i=0;i<fSize;i++) fron[Frontier[i]] = 1;
	//vertices on frontier need to process edges later
      }
      else if(dense && fSize*TOSPARSE < n) { //switch back to sparse
	dense = 0;
	_seq<intT> R = sequence::packIndex(fron,n);
	parallel_for(long i=0;i<R.n;i++)
	  Frontier[i] = R.A[i];
      }

      if(dense) { //do dense
	t5.start();
	parallel_for(intT i=0;i<n;i++) fronNext[i] = 0;
	parallel_for(intT i=0;i<n;i++) {
	  if(componentID[i] == INT_T_MAX) {//unvisited
	    vertex<intT> fv = G[i];
	    for(intT j=0;j<fv.degree;j++) {
	      intT ngh = fv.Neighbors[j];
	      if(fron[ngh]) {
		intT nghID = componentID[ngh];
		componentID[i] = nghID;
		fronNext[i] = 1;
		break; // quit early
	      }
	    }
	  }
	}
	fSize = sequence::sum(fronNext,n);
	totalVisited += fSize; round++;
	swap(fron,fronNext);
	t5.stop();
      }
      else { //do sparse
	t3.start();
	parallel_for(intT i=0; i < fSize; i++) {
	  intT v = Frontier[i];
	  vertex<intT> fv = G[v];
	  frontierVertices[i].first = v;
	  frontierVertices[i].second = fv;
	  intT d = fv.degree;
	  Degrees[i] = d;
	}
	intT nr = sequence::plusScan(Degrees,Degrees,fSize);

	t3.stop();

	t4.start();

	parallel_for (intT i=0; i<fSize; i++) {
	  intT v = frontierVertices[i].first;
	  vertex<intT> fv = frontierVertices[i].second;
	  if(fv.degree > 0) {
	    intT o = Degrees[i];
	    intT k = 0;
	    intT myID = componentID[v];
	    for (intT j=0; j < fv.degree; j++) {
	      intT ngh = fv.Neighbors[j];
	      if (componentID[ngh] == INT_T_MAX && 
		  utils::CAS_GCC(&componentID[ngh], INT_T_MAX, myID)) {
		FrontierNext[o+j] = ngh;
	      } else {
		intT nghID = componentID[ngh];
		// could have lost to someone in same component, or diff component.
		if (nghID != myID) { //keep inter-component edge
		  fv.Neighbors[k++] = -nghID-1; //do relabeling now
		}
		FrontierNext[o+j] = -1;
	      }
	    }
	    G[v].degree = k;
	  }
	}

	fSize = sequence::filter(FrontierNext,Frontier,nr,nonNegF());    
	totalVisited += fSize;
	round++;
	t4.stop();
      }
    }
    free(fron); free(fronNext);
    t6.start();
    if(decompositionRound) free(vertexPerm);

    //compute new labels in consecutive range labels contains the
    //labels of the vertices (all vertices in the same component
    //should have the same label).  flags2 maps old component ID to
    //new component ID
    intT* labels = newA(intT,n);
    intT* flags2 = Frontier; //reuse space
    parallel_for(intT i=0;i<n;i++) flags2[i] = 0;
    parallel_for(intT i=0;i<n;i++) { 
      intT id = componentID[i];
      if(!flags2[id]) flags2[id] = 1;
    }

    intT maxID = sequence::plusScan(flags2,flags2,n);

    parallel_for(intT i=0;i<n;i++) {
      labels[i] = flags2[componentID[i]]; //new ID's
    }
    
    t6.stop();
    t11.start();

    parallel_for(intT i=0;i<n;i++) {
      intT k = 0;
      intT myID = componentID[i];
      for(intT j=0;j<G[i].degree;j++)
	{
	  intT ngh = G[i].Neighbors[j];
	  if(ngh < 0) G[i].Neighbors[k++] = -ngh-1;
	  else {
	    intT nghID = componentID[ngh];
	    if(nghID != myID) G[i].Neighbors[k++] = nghID; //label ngh with ID
	  }
	}
      Degrees[i] = k;
    }

    t11.stop();
    t7.start();
    Degrees[n] = 0;
    intT new_m = sequence::plusScan(Degrees,Degrees,n+1);

    edge<intT>* edgePairs = newA(edge<intT>,new_m);

    parallel_for(intT i=0;i<n;i++) {
      intT offset = Degrees[i];
      intT d = Degrees[i+1] - offset;
      if (d > 0) {
	intT myID = componentID[i];
	for(intT j=0;j<d;j++) {
	  intT ngh = G[i].Neighbors[j];
	  edgePairs[offset+j].u = myID;
	  edgePairs[offset+j].v = ngh; //ngh was already relabeled during bfs
	}
      }
    }

    GA.del(); //will delete original edge list

    t7.stop();
    t9.start();
 
    edgeArray<intT> EA(edgePairs,maxID,maxID,new_m);
  
    //remove duplicate edges
    edgeArray<intT> newEdges = removeDuplicateEdges(EA);
    EA.del();
    t9.stop();    

    //relabel
    t10.start();   
    //get rid of singletons
    parallel_for(intT i=0;i<maxID+1;i++) {
      flags[i] = 0;
    }
    parallel_for(intT i=0;i<newEdges.nonZeros;i++) {
      newEdges.E[i].u = flags2[newEdges.E[i].u]; //relabeling 
      newEdges.E[i].v = flags2[newEdges.E[i].v]; //relabeling
      intT u = newEdges.E[i].u;
      intT v = newEdges.E[i].v;
      if(!flags[u]) flags[u] = 1;
      if(!flags[v]) flags[v] = 1;
    }

    //relabel edges after removing singletons
    sequence::plusScan(flags,flags,maxID+1);
    intT numV = flags[maxID];
    parallel_for(intT i=0;i<newEdges.nonZeros;i++) {
      newEdges.E[i].u = flags[newEdges.E[i].u];
      newEdges.E[i].v = flags[newEdges.E[i].v];
    }
    newEdges.numCols = newEdges.numRows = numV;

    intT* mapping = Degrees; //reuse space
    //keep mapping from component IDs after removing singletons to
    //original component IDs
    parallel_for(intT i=0;i<maxID;i++) {
      if(flags[i] != flags[i+1]) mapping[flags[i]] = i;
    }

    //collect, making a new, smaller graph
    GA = graphFromEdges(newEdges,false); 
    
    intT edgesRemaining = GA.m;
    intT verticesRemaining = GA.n;

    free(componentID); free(Frontier); free(FrontierNext);
    free(shifts);
    free(frontierVertices);
    t10.stop();
    //base case
    if(GA.m == 0) return labels;

    //recurse
    intT* newLabels = genDecomposition(GA,beta,decompositionRound+1);

    t8.start();
    //relabel non-singletons, since recursive call generated new labels
    parallel_for(intT i=0;i<n;i++) {
      intT oldID = flags[labels[i]];
      if(oldID != flags[labels[i]+1]) //was a non-singleton
	labels[i] = mapping[newLabels[oldID]];
    }
    free(mapping);
    free(newLabels);
    free(flags);
    t8.stop();
    return labels;
}


intT* CC(graph<intT>& GA, float beta) {
  intT n = GA.n;
  intT* labels = genDecomposition(GA, beta, 0);
  //reportAll();
  return labels;
}

