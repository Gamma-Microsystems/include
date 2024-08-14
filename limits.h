#pragma once

#define _LITTLE_ENDIAN

#define UCHAR_MAX 255
#define CHAR_BIT 8

// GPL License below :P (https://github.com/bminor/glibc/blob/master/include/limits.h)

/* Minimum and maximum values a `signed long int' can hold.  */
#  if __WORDSIZE == 64
#   define LONG_MAX	9223372036854775807L
#  else
#   define LONG_MAX	2147483647L
#  endif
#  define LONG_MIN	(-LONG_MAX - 1L)

/* Maximum value an `unsigned long int' can hold.  (Minimum is 0.)  */
#  if __WORDSIZE == 64
#   define ULONG_MAX	18446744073709551615UL
#  else
#   define ULONG_MAX	4294967295UL
#  endif

#  ifdef __USE_ISOC99

/* Minimum and maximum values a `signed long long int' can hold.  */
#   define LLONG_MAX	9223372036854775807LL
#   define LLONG_MIN	(-LLONG_MAX - 1LL)

/* Maximum value an `unsigned long long int' can hold.  (Minimum is 0.)  */
#   define ULLONG_MAX	18446744073709551615ULL

#if defined __USE_ISOC99 && defined __GNUC__
# ifndef LLONG_MIN
#  define LLONG_MIN	(-LLONG_MAX-1)
# endif
# ifndef LLONG_MAX
#  define LLONG_MAX	__LONG_LONG_MAX__
# endif
# ifndef ULLONG_MAX
#  define ULLONG_MAX	(LLONG_MAX * 2ULL + 1)
# endif
#endif

/* The integer width macros are not defined by GCC's <limits.h> before
   GCC 7, or if _GNU_SOURCE rather than
   __STDC_WANT_IEC_60559_BFP_EXT__ is used to enable this feature.  */
#if __GLIBC_USE (IEC_60559_BFP_EXT_C23)
# ifndef CHAR_WIDTH
#  define CHAR_WIDTH 8
# endif
# ifndef SCHAR_WIDTH
#  define SCHAR_WIDTH 8
# endif
# ifndef UCHAR_WIDTH
#  define UCHAR_WIDTH 8
# endif
# ifndef SHRT_WIDTH
#  define SHRT_WIDTH 16
# endif
# ifndef USHRT_WIDTH
#  define USHRT_WIDTH 16
# endif
# ifndef INT_WIDTH
#  define INT_WIDTH 32
# endif
# ifndef UINT_WIDTH
#  define UINT_WIDTH 32
# endif
# ifndef LONG_WIDTH
#  define LONG_WIDTH __WORDSIZE
# endif
# ifndef ULONG_WIDTH
#  define ULONG_WIDTH __WORDSIZE
# endif
# ifndef LLONG_WIDTH
#  define LLONG_WIDTH 64
# endif
# ifndef ULLONG_WIDTH
#  define ULLONG_WIDTH 64
# endif
#endif /* Use IEC_60559_BFP_EXT.  */
