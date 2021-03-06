// This code is part of the Problem Based Benchmark Suite (PBBS)
// Copyright (c) 2016 Guy Blelloch, Daniel Ferizovic, and the PBBS team
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

#pragma once

#include "utils.h"

namespace pbbs {

  // A cheap version of an interface that should be improved
  // Allows forking a state into multiple states
  struct random {
  public:
    random(size_t seed) : state(seed) {};
    random() : state(0) {};
    random fork(uint64_t i) {
      return random(hash64(hash64(i+state))); }
    random next() { return fork(0);}
    size_t ith_rand(uint64_t i) {
      return hash64(i+state);}
    size_t rand() { return ith_rand(0);}
  private:
    uint64_t state = 0;
  };
}
