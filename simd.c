#include <immintrin.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define RDTSC_START(cycles)                                             \
  do {                                                                \
    uint32_t cyc_high, cyc_low;                                     \
    __asm volatile("cpuid\n"                                        \
		                          "rdtsc\n"                                        \
		                          "mov %%edx, %0\n"                                \
		   "mov %%eax, %1" :                                \
		   "=r" (cyc_high),                                 \
		   "=r"(cyc_low) :                                  \
		   : /* no read only */                             \
		   "%rax", "%rbx", "%rcx", "%rdx" /* clobbers */    \
		   );                                               \
    (cycles) = ((uint64_t)cyc_high << 32) | cyc_low;                \
  } while (0)

#define RDTSC_STOP(cycles)                                              \
  do {                                                                \
    uint32_t cyc_high, cyc_low;                                     \
    __asm volatile("rdtscp\n"                                       \
		                          "mov %%edx, %0\n"                                \
		                          "mov %%eax, %1\n"                                \
		   "cpuid" :                                        \
		   "=r"(cyc_high),                                  \
		   "=r"(cyc_low) :                                  \
		   /* no read only registers */ :                   \
		   "%rax", "%rbx", "%rcx", "%rdx" /* clobbers */    \
		   );                                               \
    (cycles) = ((uint64_t)cyc_high << 32) | cyc_low;                \
  } while (0)

void dump( __m256i reg, char * msg ) {
  printf("%s ", msg);
  unsigned char c[32];
  _mm256_storeu_si256((__m256i *)c, reg);
  for( int i = 31; i >=0; i-- )
    printf("%02hhX ", c[i] );
  printf("\n");
}

#if defined(__GNUC__) && !defined(__clang__) && !defined(__ICC)

// this maps to multiple instructions in avx, but one in avx2 (?)
static inline void
__attribute__((__always_inline__))
_mm256_storeu2_m128i(__m128i* const hiaddr, __m128i* const loaddr, const __m256i a)
{
  _mm_storeu_si128(loaddr, _mm256_castsi256_si128(a));
  _mm_storeu_si128(hiaddr, _mm256_extracti128_si256(a, 1));
}
#endif

static inline void morton_vec8(uint32_t *in0, uint32_t *in1,  uint64_t *out ) {

  // nybble -> byte lookups
  __m256i m0 = _mm256_set_epi8(85, 84, 81, 80, 69, 68, 65, 64, 21, 20, 17,
		       16, 5, 4, 1, 0, 85, 84, 81, 80, 69, 68, 65,
		       64, 21, 20, 17, 16, 5, 4, 1, 0);

  __m256i m1 = _mm256_slli_epi64( m0, 1);

  __m256i nybble_mask = _mm256_set1_epi8(0x0F);

  __m256i word0 = _mm256_loadu_si256((__m256i *)in0);
  __m256i word1 = _mm256_loadu_si256((__m256i *)in1);
  
  __m256i even0 = _mm256_and_si256( word0, nybble_mask);  // 1 / .33 ?
  __m256i odd0  = _mm256_and_si256( _mm256_srli_epi64( word0, 4), nybble_mask);

  __m256i even1 = _mm256_and_si256( word1, nybble_mask);
  __m256i odd1  = _mm256_and_si256( _mm256_srli_epi64( word1, 4), nybble_mask);
  
  // lookup
    
  even0 = _mm256_shuffle_epi8( m0, even0 );  // 1 / 1
  odd0  = _mm256_shuffle_epi8( m0, odd0  );
  
  even1 = _mm256_shuffle_epi8( m1, even1 );  
  odd1  = _mm256_shuffle_epi8( m1, odd1  );

  // combine
  
  __m256i even = _mm256_or_si256( even0, even1 );  // 1 / .33
  __m256i odd  = _mm256_or_si256( odd0, odd1 );

  // byte interleave
  __m256i los = _mm256_unpacklo_epi8( even, odd );  // 1 / 1
  __m256i his = _mm256_unpackhi_epi8( even, odd );

  // AC,BD => AB,CD
  // interleaved 128i stores to 8 x uint64_t
  _mm256_storeu2_m128i ((__m128i*) (out+4),(__m128i*) out, los);
  _mm256_storeu2_m128i ((__m128i*) (out+6),(__m128i*) (out+2), his);
}

int main() {
  uint64_t start,end;
  uint32_t *in0, *in1;

#define N 1024
  
  in0 = malloc(N*sizeof(uint32_t));  in1 = malloc(N*sizeof(uint32_t));
  for(uint32_t i= 0; i < N; i++) {
    in0[i] = i;
    in1[i] = 0;
  }
  
  uint64_t  *out;
  out = malloc(N*sizeof(uint64_t));

  RDTSC_START(start);

  for(int i = 0; i < N; i+=8) 
    morton_vec8(in0+i, in1+i, out+i);

  RDTSC_STOP(end);

  printf("N=%d\ncycles = %ld\nper int=%f\n",N, end-start, (end-start)/(N*2.0));

  // prevent optimizing out
  uint64_t sum=0;
  for(int i=0; i < N; i++)
    sum += out[i];

  printf("sum %ld\n", sum);
  
  for(int i = 0; i < 8; i++ )
    printf("%ld ", out[i]);

  printf("last (%d, %d) => %ld\n", in0[N-1], in1[N-1], out[N-1]);

  printf("\n\n");
}
