/* Jody Bruchon's SIMD configuration headers
 * See zeromerge.c for license information */

#ifndef ZEROMERGE_SIMD_H
#define ZEROMERGE_SIMD_H

#ifdef __cplusplus
extern "C" {
#endif

/* Disable SIMD if not 64-bit width or not 64-bit x86 code */
#if defined NO_SIMD || !defined __x86_64__ || (defined NO_SSE2 && defined NO_AVX2)
 #undef USE_SSE2
 #undef USE_AVX2
 #ifndef NO_SIMD
  #define NO_SIMD
 #endif
#endif

/* Use SIMD by default */
#if !defined NO_SIMD
 #if defined __SSE2__ && !defined NO_SSE2
  #define USE_SSE2
 #endif
 #if defined __AVX2__ && !defined NO_AVX2
  #define USE_AVX2
 #endif
 #if defined _MSC_VER || defined _WIN32 || defined __MINGW32__
  /* Microsoft C/C++-compatible compiler */
  #include <intrin.h>
  #define aligned_alloc(a,b) _aligned_malloc(b,a)
  #define ALIGNED_FREE(a) _aligned_free(a)
 #elif (defined __GNUC__  || defined __clang__ ) && (defined __x86_64__  || defined __i386__ )
  /* GCC or Clang targeting x86/x86-64 */
  #include <x86intrin.h>
  #define ALIGNED_FREE(a) free(a)
 #endif
#endif /* !NO_SIMD */

#if defined USE_SSE2 || defined USE_AVX2
union UINT256 {
	__m256i  v256;
	__m128i  v128[2];
	uint64_t v64[4];
};

#endif

extern size_t zero_merge_avx2(size_t pos, size_t length, char *buf1, char *buf2);
extern size_t zero_merge_sse2(size_t pos, size_t length, char *buf1, char *buf2);

#ifdef __cplusplus
}
#endif

#endif	/* ZEROMERGE_SIMD_H */
