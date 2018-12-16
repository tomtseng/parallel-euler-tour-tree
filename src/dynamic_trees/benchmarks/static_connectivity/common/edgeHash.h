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

#ifndef A_EHASH_INCLUDED
#define A_EHASH_INCLUDED

#include "parallel.h"
#include "utils.h"
#include "sequence.h"
//#include "gettime.h"
using namespace std;

// A "history independent" hash table that supports insertion, and searching 
// It is described in the paper
//   Guy E. Blelloch, Daniel Golovin
//   Strongly History-Independent Hashing with Applications
//   FOCS 2007: 272-282
// At any quiescent point (when no operations are actively updating the
//   structure) the state will depend only on the keys it contains and not
//   on the history of the insertion order.
// Insertions can happen in parallel, but they cannot overlap with searches
// Searches can happen in parallel
// Deletions must happen sequentially
template <class HASH, class intT>
class ETable {
 private:
  typedef typename HASH::eType eType;
  typedef typename HASH::kType kType;
  intT m;
  intT mask;
  eType empty;
  HASH hashStruct;
  eType* TA;
  float load;

  // needs to be in separate routine due to Cilk bugs
  static void clearA(eType* A, intT n, eType v) {
    parallel_for (intT i=0; i < n; i++) A[i] = v;
  }

  struct notEmptyF { 
    eType e; notEmptyF(eType _e) : e(_e) {} 
    int operator() (eType a) {return e != a;}};

  uintT hashToRange(intT h) {return h & mask;}
  intT firstIndex(kType v) {return hashToRange(hashStruct.hash(v));}
  intT incrementIndex(intT h) {return hashToRange(h+1);}
  intT decrementIndex(intT h) {return hashToRange(h-1);}
  bool lessIndex(intT a, intT b) {return 2 * hashToRange(a - b) > m;}
  bool lessEqIndex(intT a, intT b) {return a==b || 2 * hashToRange(a - b) > m;}


 public:
  // Size is the maximum number of values the hash table will hold.
  // Overfilling the table could put it into an infinite loop.
 ETable(intT size, HASH hashF, float _load) :
  load(_load),
    m(1 << utils::log2Up(100+(intT)(_load*(float)size))), 
    mask(m-1),
    empty(hashF.empty()),
    hashStruct(hashF), 
    TA(newA(eType,m))
      { clearA(TA,m,empty); }

  // Deletes the allocated arrays
  void del() {
    free(TA); 
  }

  // prioritized linear probing
  //   a new key will bump an existing key up if it has a higher priority
  //   an equal key will replace an old key if replaceQ(new,old) is true
  // returns 0 if not inserted (i.e. equal and replaceQ false) and 1 otherwise
  bool insert(eType v) {
    intT i = firstIndex(hashStruct.getKey(v));
    while (1) {
      eType c = TA[i];
      if (c.v == empty.v) { //"empty"
	if (utils::CAS_GCC(&TA[i],c,v)) return 1;
      } else {
	int cmp = hashStruct.cmp(hashStruct.getKey(v),hashStruct.getKey(c));
	if (cmp == 0) {
	  if (!hashStruct.replaceQ(v,c)) return 0; 
	  else if (utils::CAS_GCC(&TA[i],c,v)) return 1;
	} else if (cmp == -1) 
	  i = incrementIndex(i);
	else if (utils::CAS_GCC(&TA[i],c,v)) {
	  v = c;
	  i = incrementIndex(i);
	}
      }
    }
  }

  bool find(kType v) {
    intT h = firstIndex(v);
    eType c = TA[h]; 
    while (1) {
      if (c.u == empty.u) return false; 
      else if (!hashStruct.cmp(v,hashStruct.getKey(c)))
	return true;
      h = incrementIndex(h);
      c = TA[h];
    }
  }


  // returns all the current entries compacted into a sequence
  _seq<eType> entries() {
    bool *FL = newA(bool,m);
    parallel_for (intT i=0; i < m; i++) 
      FL[i] = (TA[i].v != empty.v);//hashStruct.cmp(TA[i],empty) != 0;
    _seq<eType> R = sequence::pack(TA,FL,m);
    free(FL);
    return R;
  }
};

template <class HASH, class ET>
  _seq<ET> removeDuplicateEdges(_seq<ET> S, HASH hashF, intT numRows) {
  intT m = S.n;
  long maxSize = min<long>((long)numRows*(long)(numRows-1),m);
  ETable<HASH,intT> T(maxSize,hashF,1.5);
  ET* A = S.A;
  {parallel_for(intT i = 0; i < m; i++) { T.insert(A[i]);}}
  _seq<ET> R = T.entries();
  T.del(); 
  return R;
}

template <class intT>
int cInt(intT v, intT b) {
  return (v > b) ? 1 : ((v == b) ? 0 : -1);}

template <class intT>
struct hashE {
  typedef edge<intT> eType;
  typedef edge<intT> kType;
  eType empty() {return edge<intT>(INT_T_MAX,INT_T_MAX);}
  kType getKey(eType v) {return v;}
  intT hash(kType e) {
    return utils::hashInt(e.u) + utils::hashInt(100*e.v); }
  int cmp(kType a, kType b) {
    int c = cInt(a.u, b.u);
    return (c == 0) ? cInt(a.v,b.v) : c;
  }
  bool replaceQ(eType v, eType b) {return 0;}
};

static edgeArray<intT> removeDuplicateEdges(edgeArray<intT> EA) {
  _seq<edge<intT> > F = removeDuplicateEdges(_seq<edge<intT> >(EA.E,EA.nonZeros), hashE<intT>(), EA.numRows);
  return edgeArray<intT>(F.A,EA.numRows,EA.numRows,F.n);
}

#endif // _A_EHASH_INCLUDED
