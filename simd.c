#include <immintrin.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void dump( __m256i reg, char * msg ) {
  printf("%s ", msg);
  unsigned char c[32];
  _mm256_storeu_si256(c, reg);
  for( int i = 31; i >=0; i-- )
    printf("%02hhX ", c[i] );
  printf("\n");
}
	  
static inline __m256i expand(__m256i word0, __m256i word1,  __m256i *out1, __m256i *out2 ) {

  const __m256i m0 = _mm256_set_epi8(85, 84, 81, 80, 69, 68, 65, 64, 21, 20, 17,
				    16, 5, 4, 1, 0, 85, 84, 81, 80, 69, 68, 65,
				    64, 21, 20, 17, 16, 5, 4, 1, 0);

  const __m256i m1 = _mm256_slli_epi64( m0, 1);

  const __m256i nybble_mask = _mm256_set1_epi8(0x0F);

  dump( m0, "m0   ");
  dump( m1, "m1   ");
  __m256i even0 = _mm256_and_si256( word0, nybble_mask);  // 1 / .33 ?
  __m256i odd0  = _mm256_and_si256( _mm256_srli_epi64( word0, 4), nybble_mask);

  __m256i even1 = _mm256_and_si256( word1, nybble_mask);
  __m256i odd1  = _mm256_and_si256( _mm256_srli_epi64( word1, 4), nybble_mask);
  dump( even1, "even1");
  
  // lookup
    
  even0 = _mm256_shuffle_epi8( m0, even0 );  // 1 / 1
  odd0  = _mm256_shuffle_epi8( m0, odd0  );
  
  even1 = _mm256_shuffle_epi8( m1, even1 );  
  odd1  = _mm256_shuffle_epi8( m1, odd1  );
  dump( even1, "even1");

  // combine
  
  __m256i even = _mm256_or_si256( even0, even1 );  // 1 / .33
  __m256i odd  = _mm256_or_si256( odd0, odd1 );

  // byte interleave
  __m256i los = _mm256_unpacklo_epi8( even, odd );  // 1 / 1
  __m256i his = _mm256_unpackhi_epi8( even, odd );

  // 128i interleave
  *out1 = _mm256_permute2x128_si256( los, his, 0x20);  // 3 / 1 
  *out2 = _mm256_permute2x128_si256( los, his, 0x31);

  dump(*out1, "lo");
  dump(*out2, "hi");
}

int main() {
  uint32_t *in0, *in1;
  in0 = malloc(32);  in1 = malloc(32);
  for(int i= 0; i < 8; i++) {
    in0[i] = i;
    in1[i] = 0;
  }
  
  uint32_t out1[8], out2[8];
  __m256i x0 = _mm256_loadu_si256(in0);
  __m256i x1 = _mm256_loadu_si256(in1);
  __m256i out1i, out2i;

  //_mm256_lddqu_si256
  expand( x1, x0, &out1i, &out2i );
  _mm256_storeu_si256( out1, out1i );
  
  for(int i = 0; i < 8; i++ )
    printf("%d ", out1[i]);
}
