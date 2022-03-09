#include <stdlib.h>
#include <string.h>

typedef VAR T;
typedef unsigned VAR UT;
typedef size_t U;
#define LIKELY(X) __builtin_expect(X,1)
#define RARE(X) __builtin_expect(X,0)

// Minimum size to steal from buffer
static const U BLOCK = 16;

// The main attraction. Sort array of ints with length n.
// Available aux space is na values
// Last npiv values of x are pivot candidates
int FUNC(rh_sort)(T *x, U n, T *aux, U na, U npiv, CMPFUNC *cmp) {
  if (n > 1<<16) return 0;              // Partitioning is better

  // Find the range.
  T min=x[-1], max=x[n];
  U r = (U)(UT)(max-min);               // Size of range
  U sh = 0;                             // Contract to fit range
  while (r>5*n) { sh++; r>>=1; }        // Shrink to stay at O(n) memory

  // Goes down to BLOCK once we know we have to merge
  U threshold = 2*BLOCK;
  U sz = r + threshold;                 // Buffer size
  if (sz>na) return 0;                  // Not enough space
  #define POS(E) ((U)(UT)((E)-min) >> sh)
  // Statistical check to make sure it's not too clumpy
  for (U i=n-npiv+1, prev=POS(x[i-1]), score=0; i<n; i++) {
    U next=POS(x[i]), d=next-prev; prev=next;
    if (d<16) { score+=16-d; if (score>=60+npiv/6) return 0; }
  }

  // Allocate buffer, and fill with sentinels
  // Sentinel value: the buffer swallows these but count recovers them
  T s = max;
  for (U i=0; i<sz; i++) aux[i] = s;

  T *xb=x;  // Stolen blocks go to xb

  // Main loop: insert array entries into buffer
  for (U i=0; i<n; i++) {
    T e = x[i];               // Entry to be inserted
    U j = POS(e);             // Target position
    T h = aux[j];             // What's there?
    // Common case is that it's empty (marked with sentinel s)
    if (LIKELY(h==s)) { aux[j]=e; continue; }

    // Collision: find size of chain and position in it
    // Reposition elements after e branchlessly during the search
    U j0=j, f=j;
    do {
      T n = aux[++f];  // Might write over this
      int c = e>=h;    // If we have to move past that entry
      j += c;          // Increments until e's final location found
      aux[f-c] = h;    // Reposition h
      h = n;
    } while (h!=s); // Until the end of the chain
    aux[j] = e;
    f += 1;  // To account for just-inserted e

    // Bad collision: send chain back to x
    if (RARE(f-j0 >= threshold)) {
      threshold = BLOCK;
      // Find the beginning of the chain (required for stability)
      while (j0 && aux[j0-1]!=s) j0--;
      // Move as many blocks from it as possible
      T *hj = aux+j0, *hf = aux+f;
      while (hj <= hf-BLOCK) {
        for (U i=0; i<BLOCK; i++) { xb[i]=hj[i]; hj[i]=s; }
        hj += BLOCK; xb += BLOCK;
      }
      // Leftover elements might have to move backwards
      U pr = j0;
      while (hj < hf) {
        e = *hj; *hj++ = s;
        U pp = POS(e);
        pr = pp>pr ? pp : pr;
        aux[pr++] = e;
      }
    }
  }
  #undef POS

  // Move all values from the buffer back to the array
  // Use xt += to convince the compiler to make it branchless
  while (aux[--sz] == s); sz++;
  T *xt=xb;
  {
    static const U u=8;  // Unrolling size
    #define WR(I) xt += s!=(*xt=aux[i+I])
    U i=0;
    for (; i<(sz&~(u-1)); i+=u) { WR(0); WR(1); WR(2); WR(3); WR(4); WR(5); WR(6); WR(7); }
    for (; i<sz; i++) WR(0);
    #undef WR
  }
  // Recover maximum/sentinel elements based on total count
  while (xt < x+n) *xt++ = s;

  // Merge stolen blocks back in if necessary
  U l = xb-x;  // Size of those blocks
  if (l) {
    // Sort x[0..l]
    FUNC(quad_merge)(x, aux, n, n, BLOCK, cmp);
    // And merge with the rest of x
    FUNC(partial_backward_merge)(x, aux, n, l, cmp);
  }
  return 1;
}
