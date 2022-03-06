typedef VAR T;
typedef size_t U;

// Counting sort of the n values starting at x
static void count_sort(T *x, U n, U *count, T min, U range) {
  memset(count, 0, range*sizeof(U));
  if (range < n/8) { // Short range: branching on count is cheap
    // Count the values
    for (U i=0; i<n; i++) count[x[i]-min]++;
    // Write based on the counts
    for (U i=0; i<range; i++)
      for (U j=0; j<count[i]; j++)
        *x++ = min+i;
  } else {
    // Count, and zero the array
    for (U i=0; i<n; i++) { count[x[i]-min]++; x[i]=0; }
    // Write differences to x
    x[0] = min;
    U end=range-1; while (count[end]==0) end--; // Assume n>0
    for (U i=0, s=0; i<end; i++) { s+=count[i]; x[s]++; }
    // Prefix sum
    { U i=0;
      for (; i+4<n; i+=4) { x[i+4] += x[i+3] += x[i+2] += x[i+1] += x[i]; }
      for (; i+1<n; i++) { x[i+1] += x[i]; }
    }
  }
}
