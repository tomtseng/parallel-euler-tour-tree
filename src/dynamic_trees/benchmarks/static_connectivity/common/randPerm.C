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
#include "parallel.h"
#include "utils.h"
#include "randPerm.h"
#include <iostream>
#include "sequence.h"
#include "speculative_for2.h"
using namespace std;

template <class E>
struct randPermStep {
  typedef pair<E,intT> pairInt;

  pairInt *dataCheck;
  intT* H;

  randPermStep(intT* _H, pairInt* _dataCheck) : 
    H(_H), dataCheck(_dataCheck) {}

  inline bool reserve(intT i) {
    reserveLoc(dataCheck[i].second,i);
    reserveLoc(dataCheck[H[i]].second, i);
    return 1;
  }

  inline bool commit (intT i) {
    intT h = H[i];
    if(dataCheck[h].second == i) {
      if(dataCheck[i].second == i) {
	swap(dataCheck[i].first, dataCheck[h].first);
	dataCheck[h].second = INT_T_MAX;
	return 1;
      }
      dataCheck[h].second = INT_T_MAX;
    }
    return 0;
  }
};

template <class E>
void randPerm(E *A, intT n, intT r) {
  
  typedef pair<E,intT> pairInt;

  intT *H = newA(intT,n);
  //intT *check = newA(intT,n);
  pairInt* dataCheck = newA(pairInt,n);

  {parallel_for (intT i=0; i < n; i++) {
    H[i] = i+utils::hash(i)%(n-i);
    dataCheck[i] = make_pair(A[i],i);
    }}

  randPermStep<E> rStep(H, dataCheck);
  speculative_for(rStep, 0, n, (r != -1) ? r : 50, 0);

  {parallel_for (intT i=0;i<n;i++) A[i] = dataCheck[i].first;}

  free(H); free(dataCheck);
}

template void randPerm<intT>(intT*,intT,intT);
template void randPerm<double>(double*,intT,intT);
