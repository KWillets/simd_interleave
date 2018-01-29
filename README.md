# simd_interleave

This is an attempt to get the fastest code to interleave bits from two source arrays of integers.  The problem was posed on [Daniel Lemire's Blog](https://lemire.me/blog/2018/01/09/how-fast-can-you-bit-interleave-32-bit-integers-simd-edition/).  

This version redefines the problem slightly to make it easier -- instead of merging pairs of consecutive int's, we take them from two separate arrays.  

The algorithm is, like others, based on interleaving zero bits in each source, and then merging them via bitwise or.  

The zero-interleaving is accomplished by mapping 4-bit units (nybbles) to bytes from a lookup table using the pshufb instruction.  Two vectors of nybbles ("even" and "odd") are extracted from each source, mapped to bytes, and then the bytes are interleaved to get the output word -- the key is that byte-level operations are all that is needed after the mapping.

To interleave two inputs, the bitwise or is done immediately after the nybble-to-byte mapping in each vector, and then the same bytewise interleaving is applied to the merged bytes.  

