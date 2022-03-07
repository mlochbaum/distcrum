# Distribution crumsort

This repository contains an in-progress effort to give [crumsort](https://github.com/scandum/crumsort) access to powerful distribution sorting algorithms when possible. Currently only 4-byte sorting has been augmented in this way. The following methods are used:

| Method                                                             | Memory       | Requires     | Top speed | Adaptive
|--------------------------------------------------------------------|--------------|--------------|-----------|---------
| [Counting sort](https://github.com/mlochbaum/rhsort#counting-sort) | range        |              | ~1ns/v    | no
| Packed radix sort                                                  | 2KB          | range <=2^16 | ~4ns/v    | no
| [Robin Hood Sort](https://github.com/mlochbaum/rhsort)             | >=2.5*length | uniformity   | ~5ns/v    | yes

Packed radix sort works like [ska_sort_copy](https://probablydance.com/2016/12/02/investigating-radix-sort/), but takes advantage of values fitting in a 2-byte range by packing them into two bytes each and sorting in the space freed this way. Robin Hood Sort is backed by quadsort, but can be somewhat slower than fluxsort when the array contains enough values that are close together. It's used only when a scan of pivot candidates indicates this is unlikely.
