#include <stdint.h>

void radpack32(int *x32, U n, int min) {
  uint16_t *x16 = (uint16_t*)x32;
  uint16_t *aux = x16 + n;
  for (size_t i=0; i<n; i++) x16[i] = (uint16_t)(x32[i] - min);
  static char cc[8*256];
  if (n < 256) {
    memset(cc, 0, 2*256);
    uint8_t *count0 = (uint8_t*)cc;
    uint8_t *count1 = count0 + 256;

    for (U i=0; i<n; i++) {
      uint16_t key = x16[i];
      count0[(uint8_t)(key     )]++;
      count1[(uint8_t)(key >> 8)]++;
    }
    uint64_t sum0 = 0, sum1 = 0;
    uint64_t *c0 = (uint64_t*)count0, *c1 = (uint64_t*)count1;
    for (U i=0; i<256/8; i++) {
      uint64_t prev0 = sum0, prev1 = sum1;
      sum0 += c0[i]; sum1 += c1[i];
      #define SH(k) sum0+=sum0<<k; sum1+=sum1<<k
      SH(8); SH(16); SH(32);
      #undef SH
      c0[i] = prev0 | (sum0<<8); c1[i] = prev1 | (sum1<<8);
      sum0 >>= 56; sum1 >>= 56;
    }
    for (U i=0; i<n; i++) { uint16_t v = x16[i]; aux[count0[(uint8_t) v    ]++] = v; }
    for (U i=0; i<n; i++) { uint16_t v = aux[i]; x16[count1[(uint8_t)(v>>8)]++] = v; }
  } else {
    memset(cc, 0, 8*256);
    uint32_t *count0 = (uint32_t*)cc;
    uint32_t *count1 = count0 + 256;

    for (U i=0; i<n; i++) {
      uint16_t key = x16[i];
      count0[(uint8_t)(key     )]++;
      count1[(uint8_t)(key >> 8)]++;
    }
    uint32_t sum0 = 0, sum1 = 0;
    for (U i=0; i<256; i++) {
      uint32_t prev0 = sum0, prev1 = sum1;
      sum0 += count0[i]; sum1 += count1[i];
      count0[i] = prev0; count1[i] = prev1;
    }
    for (U i=0; i<n; i++) { uint16_t v = x16[i]; aux[count0[(uint8_t) v    ]++] = v; }
    for (U i=0; i<n; i++) { uint16_t v = aux[i]; x16[count1[(uint8_t)(v>>8)]++] = v; }
  }
  for (size_t i=n; i--; ) x32[i] = min + (uint32_t)(x16[i]);
}
